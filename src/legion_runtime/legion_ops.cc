/* Copyright 2014 Stanford University
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include "runtime.h"
#include "legion_ops.h"
#include "legion_tasks.h"
#include "region_tree.h"
#include "legion_spy.h"
#include "legion_trace.h"
#include "legion_logging.h"
#include "legion_profiling.h"

#include "naut_debug.h"

namespace LegionRuntime {
  namespace HighLevel {

    // Extern declarations for loggers
    extern Logger::Category log_run;
    extern Logger::Category log_task;
    extern Logger::Category log_region;
    extern Logger::Category log_index;
    extern Logger::Category log_field;
    extern Logger::Category log_inst;
    extern Logger::Category log_spy;
    extern Logger::Category log_garbage;
    extern Logger::Category log_leak;
    extern Logger::Category log_variant;

    /////////////////////////////////////////////////////////////
    // Operation 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    Operation::Operation(Runtime *rt)
      : runtime(rt), op_lock(Reservation::create_reservation()), 
        gen(0), unique_op_id(0), 
        outstanding_mapping_deps(0),
        outstanding_commit_deps(0),
        outstanding_mapping_references(0),
        mapped(false), hardened(false), completed(false), 
        committed(false), parent_ctx(NULL)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    Operation::~Operation(void)
    //--------------------------------------------------------------------------
    {
      op_lock.destroy_reservation();
      op_lock = Reservation::NO_RESERVATION;
    }

    //--------------------------------------------------------------------------
    void Operation::activate_operation(void)
    //--------------------------------------------------------------------------
    {
      // Get a new unique ID for this operation
      unique_op_id = runtime->get_unique_operation_id();
      outstanding_mapping_deps = 0;
      outstanding_speculation_deps = 0;
      outstanding_commit_deps = 0;
      outstanding_mapping_references = 0;
      mapped = false;
      executed = false;
      resolved = false;
      hardened = false;
      completed = false;
      committed = false;
      trigger_mapping_invoked = false;
      trigger_resolution_invoked = false;
      trigger_complete_invoked = false;
      trigger_commit_invoked = false;
      track_parent = false;
      parent_ctx = NULL;
      children_mapped = Event::NO_EVENT;
      need_completion_trigger = true;
      completion_event = UserEvent::create_user_event();
      trace = NULL;
      tracing = false;
      must_epoch = NULL;
      must_epoch_gen = 0;
      must_epoch_index = 0;
#ifdef DEBUG_HIGH_LEVEL
      assert(completion_event.exists());
#endif
    }
    
    //--------------------------------------------------------------------------
    void Operation::deactivate_operation(void)
    //--------------------------------------------------------------------------
    {
      {
        AutoLock o_lock(op_lock);
        gen++;
      }
      incoming.clear();
      outgoing.clear();
      unverified_regions.clear();
      verify_regions.clear();
      dependent_children_mapped.clear();
      if (need_completion_trigger && !completion_event.has_triggered())
        completion_event.trigger();
    }

    //--------------------------------------------------------------------------
    unsigned Operation::get_operation_depth(void) const
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_HIGH_LEVEL
      assert(parent_ctx != NULL);
#endif
      return (parent_ctx->depth+1);
    }

    //--------------------------------------------------------------------------
    void Operation::initialize_privilege_path(RegionTreePath &path,
                                              const RegionRequirement &req)
    //--------------------------------------------------------------------------
    {
      if ((req.handle_type == SINGULAR) || (req.handle_type == REG_PROJECTION))
      {
        runtime->forest->initialize_path(req.region.get_index_space(),
                                         req.parent.get_index_space(), path);
      }
      else
      {
#ifdef DEBUG_HIGH_LEVEL
        assert(req.handle_type == PART_PROJECTION);
#endif
        runtime->forest->initialize_path(req.partition.get_index_partition(),
                                         req.parent.get_index_space(), path);
      }
    }

    //--------------------------------------------------------------------------
    void Operation::initialize_mapping_path(RegionTreePath &path,
                                            const RegionRequirement &req,
                                            LogicalRegion start_node)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_HIGH_LEVEL
      assert(req.handle_type == SINGULAR);
#endif
      runtime->forest->initialize_path(req.region.get_index_space(),
                                       start_node.get_index_space(), path);
    }

    //--------------------------------------------------------------------------
    void Operation::initialize_mapping_path(RegionTreePath &path,
                                            const RegionRequirement &req,
                                            LogicalPartition start_node)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_HIGH_LEVEL
      assert(req.handle_type == SINGULAR);
#endif
      runtime->forest->initialize_path(req.region.get_index_space(),
                                       start_node.get_index_partition(), path);
    }

    //--------------------------------------------------------------------------
    void Operation::set_trace(LegionTrace *t)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_HIGH_LEVEL
      assert(trace == NULL);
      assert(t != NULL);
#endif
      trace = t; 
      tracing = !trace->is_fixed();
    }

    //--------------------------------------------------------------------------
    void Operation::set_must_epoch(MustEpochOp *epoch, unsigned index)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_HIGH_LEVEL
      assert(must_epoch == NULL);
      assert(epoch != NULL);
#endif
      must_epoch = epoch;
      must_epoch_gen = epoch->get_generation();
      must_epoch_index = index;
      must_epoch->register_subop(this);
    }

    //--------------------------------------------------------------------------
    /*static*/ void Operation::localize_region_requirement(RegionRequirement &r)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_HIGH_LEVEL
      assert(r.handle_type == SINGULAR);
#endif
      r.parent = r.region;
      r.prop = EXCLUSIVE;
    }

    //--------------------------------------------------------------------------
    void Operation::initialize_operation(SingleTask *ctx, bool track, 
                                         Event child_event, 
                                         unsigned regs/*= 0*/)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_HIGH_LEVEL
      assert(ctx != NULL);
      assert(completion_event.exists());
#endif
      NAUTILUS_DEEP_DEBUG("in initialize operation\n");
      parent_ctx = ctx;
      track_parent = track;
      children_mapped = child_event;
      NAUTILUS_DEEP_DEBUG("track parent: %u\n", track_parent);
      if (track_parent) {
          NAUTILUS_DEEP_DEBUG("register child operation\n");
        parent_ctx->register_child_operation(this);
      }
      for (unsigned idx = 0; idx < regs; idx++) {
          NAUTILUS_DEEP_DEBUG("unverified regions insert %u\n", idx);
        unverified_regions.insert(idx);
      }

      NAUTILUS_DEEP_DEBUG("initialize operation\n");
    }

    //--------------------------------------------------------------------------
    void Operation::trigger_dependence_analysis(void)
    //--------------------------------------------------------------------------
    {
      begin_dependence_analysis();
      end_dependence_analysis();
      // Since there are no registered dependences here
      // then trigger mapping will occur immediately
    }

    //--------------------------------------------------------------------------
    void Operation::trigger_mapping(void)
    //--------------------------------------------------------------------------
    {
      // Then put this thing on the ready queue
      runtime->add_to_local_queue(parent_ctx->get_executing_processor(),
                                  this, false/*prev fail*/);
    }

    //--------------------------------------------------------------------------
    bool Operation::trigger_execution(void)
    //--------------------------------------------------------------------------
    {
      // Mark that we finished mapping
      complete_mapping();
      // If we have nothing to do also mark that we have completed execution
      complete_execution();
      // Return true indicating we successfully triggered
      return true;
    }
    
    //--------------------------------------------------------------------------
    void Operation::trigger_resolution(void)
    //--------------------------------------------------------------------------
    {
      resolve_speculation();
    }

    //--------------------------------------------------------------------------
    void Operation::deferred_complete(void)
    //--------------------------------------------------------------------------
    {
      // should only be called if overridden
      assert(false);
    }

    //--------------------------------------------------------------------------
    void Operation::trigger_complete(void)
    //--------------------------------------------------------------------------
    {
#ifdef LEGION_LOGGING
      LegionLogging::log_timing_event(Machine::get_executing_processor(),
                                      unique_op_id, COMPLETE_OPERATION);
#endif
      complete_operation();
    }

    //--------------------------------------------------------------------------
    void Operation::trigger_commit(void)
    //--------------------------------------------------------------------------
    {
      commit_operation();
      // Once we're done with this, we can deactivate the object
      deactivate();
    }

    //--------------------------------------------------------------------------
    void Operation::report_aliased_requirements(unsigned idx1, unsigned idx2)
    //--------------------------------------------------------------------------
    {
      // should only be called if overridden
      assert(false);
    }

    //--------------------------------------------------------------------------
    void Operation::complete_mapping(void)
    //--------------------------------------------------------------------------
    {
      // Mark that we are mapped and make a copy of the outgoing that
      // we can read since people can still add outgoing dependences
      // even after we have mapped.
      bool need_resolution = false;
      bool need_complete = false;
      std::map<Operation*,GenerationID> outgoing_copy;
      bool use_copy;
      {
        AutoLock o_lock(op_lock);
#ifdef DEBUG_HIGH_LEVEL
        assert(!mapped);
#endif
        mapped = true;
        use_copy = (outstanding_mapping_references > 0);
        if (use_copy)
          outgoing_copy = outgoing;
        if (executed && !resolved && (outstanding_speculation_deps == 0) && 
            !trigger_resolution_invoked)
        {
          trigger_resolution_invoked = true;
          need_resolution = true;
        }
        if (executed && resolved && !trigger_complete_invoked)
        {
          trigger_complete_invoked = true;
          need_complete = true;
        }
      }
      if (need_resolution)
        trigger_resolution();
      if (need_complete)
        trigger_complete();
      if (use_copy)
      {
        for (std::map<Operation*,GenerationID>::const_iterator it = 
              outgoing_copy.begin(); it != outgoing_copy.end(); it++)
        {
          it->first->notify_mapping_dependence(it->second);
        }
      }
      else
      {
        for (std::map<Operation*,GenerationID>::const_iterator it = 
              outgoing.begin(); it != outgoing.end(); it++)
        {
          it->first->notify_mapping_dependence(it->second);
        }
      }
    }

    //--------------------------------------------------------------------------
    void Operation::complete_execution(void)
    //--------------------------------------------------------------------------
    {
      bool need_resolution = false;
      bool need_complete = false;
      // Tell our parent context that we are done mapping
      // It's important that this is done before we mark that we
      // are executed to avoid race conditions
      if (track_parent)
        parent_ctx->register_child_executed(this);
      {
        AutoLock o_lock(op_lock);
#ifdef DEBUG_HIGH_LEVEL
        assert(!executed);
#endif
        executed = true;
        // If we haven't been resolved and we've already mapped, check to see
        // if all of our speculation deps have been satisfied
        if (mapped && !resolved && (outstanding_speculation_deps == 0) &&
            !trigger_resolution_invoked)
        {
          trigger_resolution_invoked = true;
          need_resolution = true;
        }
        if (mapped && resolved && !trigger_complete_invoked)
        {
          trigger_complete_invoked = true;
          need_complete = true;
        }
      }
      if (need_resolution)
        trigger_resolution();
      if (need_complete)
        trigger_complete();
    }

    //--------------------------------------------------------------------------
    void Operation::resolve_speculation(void)
    //--------------------------------------------------------------------------
    {
#ifdef LEGION_LOGGING
      LegionLogging::log_timing_event(Machine::get_executing_processor(),
                                      unique_op_id, RESOLVE_SPECULATION);
#endif
      // Mark that we are mapped and make a copy of the outgoing
      // edges that we can read since people can still be adding
      // outgoing dependences even after we have resolved.
      std::map<Operation*,GenerationID> outgoing_copy;
      bool use_copy;
      bool need_trigger = false;
      {
        AutoLock o_lock(op_lock);
#ifdef DEBUG_HIGH_LEVEL
        assert(!resolved);
#endif
        resolved = true;
        use_copy = (outstanding_mapping_references > 0);
        if (use_copy)
          outgoing_copy = outgoing;
        if (mapped && executed && !trigger_complete_invoked)
        {
          trigger_complete_invoked = true;
          need_trigger = true;
        }
      }
      if (use_copy)
      {
        for (std::map<Operation*,GenerationID>::const_iterator it =
              outgoing_copy.begin(); it != outgoing_copy.end(); it++)
        {
          it->first->notify_speculation_dependence(it->second);
        }
      }
      else
      {
        for (std::map<Operation*,GenerationID>::const_iterator it =
              outgoing.begin(); it != outgoing.end(); it++)
        {
          it->first->notify_speculation_dependence(it->second);
        }
      }
      if (need_trigger)
        trigger_complete();
    }

    //--------------------------------------------------------------------------
    void Operation::complete_operation(void)
    //--------------------------------------------------------------------------
    {
      bool need_trigger = false;
      // Tell our parent that we are complete
      // It's important that we do this before we mark ourselves
      // completed in order to avoid race conditions
      if (track_parent)
        parent_ctx->register_child_complete(this);
      {
        AutoLock o_lock(op_lock);
#ifdef DEBUG_HIGH_LEVEL
        assert(mapped);
        assert(executed);
        assert(resolved);
        assert(!completed);
#endif
        completed = true;
        // Check to see if we need to trigger commit
        if (!trigger_commit_invoked && ((!Runtime::resilient_mode) ||
            ((hardened && unverified_regions.empty()) ||
            ((outstanding_mapping_references == 0) &&
             (outstanding_commit_deps == 0)))))
        {
          trigger_commit_invoked = true;
          need_trigger = true;
        }
      }
      if (need_completion_trigger)
        completion_event.trigger(); 
      if (must_epoch != NULL)
        must_epoch->notify_subop_complete(this);
      // finally notify all the operations we dependended on
      // that we validated their regions note we don't need
      // the lock since this was all set when we did our mapping analysis
      for (std::map<Operation*,std::set<unsigned> >::const_iterator it =
            verify_regions.begin(); it != verify_regions.end(); it++)
      {
#ifdef DEBUG_HIGH_LEVEL
        assert(incoming.find(it->first) != incoming.end());
#endif
        GenerationID ver_gen = incoming[it->first];
        it->first->notify_regions_verified(it->second, ver_gen);
      } 
      // If we're not in resilient mode, then we can now
      // commit this operation
      if (need_trigger)
        trigger_commit();
    }

    //--------------------------------------------------------------------------
    void Operation::commit_operation(void)
    //--------------------------------------------------------------------------
    {
#ifdef LEGION_LOGGING
      LegionLogging::log_timing_event(Machine::get_executing_processor(),
                                      unique_op_id, COMMIT_OPERATION);
#endif
      // Tell our parent context that we are committed
      // Do this before actually committing to avoid race conditions
      if (track_parent)
        parent_ctx->register_child_commit(this);
      // Mark that we are committed 
      {
        AutoLock o_lock(op_lock);
#ifdef DEBUG_HIGH_LEVEL
        assert(mapped);
        assert(executed);
        assert(resolved);
        assert(completed);
        assert(!committed);
#endif
        committed = true;
      } 
      if (must_epoch != NULL)
        must_epoch->notify_subop_commit(this);
      // Finally tell any incoming edges that we've now committed
      for (std::map<Operation*,GenerationID>::const_iterator it = 
            incoming.begin(); it != incoming.end(); it++)
      {
        it->first->notify_commit_dependence(it->second);
      }
    }

    //--------------------------------------------------------------------------
    void Operation::harden_operation(void)
    //--------------------------------------------------------------------------
    {
      // Mark that this operation is now hardened against failures
      bool need_trigger = false;
      {
        AutoLock o_lock(op_lock);
#ifdef DEBUG_HIGH_LEVEL
        assert(!hardened);
#endif
        hardened = true;
        if (completed && unverified_regions.empty() && 
            !trigger_commit_invoked)
        {
          trigger_commit_invoked = true;
          need_trigger = true;
        }
      }
      if (need_trigger)
        trigger_commit();
    }

    //--------------------------------------------------------------------------
    void Operation::quash_operation(GenerationID gen, bool restart)
    //--------------------------------------------------------------------------
    {
      // TODO: actually handle quashing of operations
      assert(false);
    }

    //--------------------------------------------------------------------------
    bool Operation::request_early_commit(void)
    //--------------------------------------------------------------------------
    {
      bool result = false;
      {
        AutoLock o_lock(op_lock);
        if (!trigger_commit_invoked)
        {
          trigger_commit_invoked = true;
          result = true;
        }
      }
      return result;
    }

    //--------------------------------------------------------------------------
    void Operation::begin_dependence_analysis(void)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_HIGH_LEVEL
      assert(outstanding_mapping_deps == 0);
#endif
      // No need to hold the lock since we haven't started yet
      outstanding_mapping_deps++;
      outstanding_speculation_deps++;
      // See if we have any fence dependences
      parent_ctx->register_fence_dependence(this);
      // Register ourselves with our trace if there is one
      // This will also add any necessary dependences
      if (trace != NULL)
        trace->register_operation(this, gen);
    }

    //--------------------------------------------------------------------------
    void Operation::end_dependence_analysis(void)
    //--------------------------------------------------------------------------
    {
      bool need_mapping = false;
      bool need_resolution = false;
      {
        AutoLock o_lock(op_lock);
#ifdef DEBUG_HIGH_LEVEL
        assert(outstanding_mapping_deps > 0);
#endif
        outstanding_mapping_deps--;
        outstanding_speculation_deps--;
        if (!mapped && (outstanding_mapping_deps == 0) &&
            !trigger_mapping_invoked)
        {
          trigger_mapping_invoked = true;
          need_mapping = true;
        }
        if (!resolved && (outstanding_speculation_deps == 0) &&
            !trigger_resolution_invoked)
        {
          trigger_resolution_invoked = true;
          need_resolution = true;
        }
      }
      if (need_mapping)
      {
        bool trigger_now = false;
        if (!dependent_children_mapped.empty())
        {
          Event wait_on = Event::merge_events(dependent_children_mapped);
          if (wait_on.exists())
          {
            DeferredMappingArgs args;
            args.hlr_id = HLR_DEFERRED_MAPPING_ID;
            args.proxy_this = this;
            args.must_epoch = must_epoch;
            args.must_epoch_gen = must_epoch_gen;
            Processor util = runtime->find_utility_group();
            util.spawn(HLR_TASK_ID, &args, sizeof(args), wait_on);
          }
          else
            trigger_now = true;
        }
        else
          trigger_now = true;
        // See if we should do the trigger now
        if (trigger_now)
        {
          if (must_epoch == NULL)
            trigger_mapping();
          else
            must_epoch->notify_mapping_dependence(must_epoch_gen);
        }
      }
      if (need_resolution)
        trigger_resolution();
    }

    //--------------------------------------------------------------------------
    bool Operation::register_dependence(Operation *target, 
                                        GenerationID target_gen)
    //--------------------------------------------------------------------------
    {
      if (must_epoch != NULL)
        must_epoch->verify_dependence(this, gen, target, target_gen);
      // The rest of this method is the same as the one below
      if (target == this)
      {
        // Can't remove this if we are tracing
        if (tracing)
        {
          // Don't forget to record the dependence
#ifdef DEBUG_HIGH_LEVEL
          assert(trace != NULL);
#endif
          if (target_gen < gen)
            trace->record_dependence(this, target_gen, this, gen);
          return false;
        }
        else
          return (target_gen < gen);
      }
      bool registered_dependence = false;
      AutoLock o_lock(op_lock);
      Event all_mapped = Event::NO_EVENT;
      bool prune = target->perform_registration(target_gen, this, gen,
                                                registered_dependence,
                                                outstanding_mapping_deps,
                                                outstanding_speculation_deps,
                                                all_mapped);
      if (all_mapped.exists())
        dependent_children_mapped.insert(all_mapped);
      if (registered_dependence)
        incoming[target] = target_gen;
      if (tracing)
      {
#ifdef DEBUG_HIGH_LEVEL
        assert(trace != NULL);
#endif
        trace->record_dependence(target, target_gen, this, gen);
        // Unsound to prune when tracing
        prune = false;
      }
      return prune;
    }

    //--------------------------------------------------------------------------
    bool Operation::register_dependence(unsigned idx, Operation *target, 
                                        GenerationID target_gen, 
                                        unsigned target_idx,
                                        DependenceType dtype)
    //--------------------------------------------------------------------------
    {
      bool do_registration = true;
      if (must_epoch != NULL)
      {
        do_registration = 
          must_epoch->record_dependence(this, gen, target, target_gen, 
                                        idx, target_idx, dtype);
      }
      // Can never register a dependence on ourself since it means
      // that the target was recycled and will never register. Return
      // true if the generation is older than our current generation.
      if (target == this)
      {
        if (target_gen == gen)
          report_aliased_requirements(target_idx, idx);
        // Can't remove this if we are tracing
        if (tracing)
        {
          // Don't forget to record the dependence
#ifdef DEBUG_HIGH_LEVEL
          assert(trace != NULL);
#endif
          if (target_gen < gen)
            trace->record_dependence(this, target_gen, this, gen,
                                     target_idx, idx, dtype);
          return false;
        }
        else
          return (target_gen < gen);
      }
      bool registered_dependence = false;
      AutoLock o_lock(op_lock);
      bool prune = false;
      if (do_registration)
      {
        Event all_mapped = Event::NO_EVENT;
        prune = target->perform_registration(target_gen, this, gen,
                                                registered_dependence,
                                                outstanding_mapping_deps,
                                                outstanding_speculation_deps,
                                                all_mapped);
        if (all_mapped.exists())
          dependent_children_mapped.insert(all_mapped);
      }
      if (registered_dependence)
        incoming[target] = target_gen;
      if (tracing)
      {
#ifdef DEBUG_HIGH_LEVEL
        assert(trace != NULL);
#endif
        trace->record_dependence(target, target_gen, this, gen,
                                 target_idx, idx, dtype);
        // Unsound to prune when tracing
        prune = false;
      }
      return prune;
    }

    //--------------------------------------------------------------------------
    bool Operation::register_region_dependence(unsigned idx, Operation *target,
                                          GenerationID target_gen, 
                                          unsigned target_idx,
                                          DependenceType dtype)
    //--------------------------------------------------------------------------
    {
      bool do_registration = true;
      if (must_epoch != NULL)
      {
        do_registration = 
          must_epoch->record_dependence(this, gen, target, target_gen, 
                                        idx, target_idx, dtype);
      }
      if (target == this)
      {
        if (target_gen == gen)
          report_aliased_requirements(target_idx, idx);
        // Can't remove this if we are tracing
        if (tracing)
        {
          // Don't forget to record the dependence
#ifdef DEBUG_HIGH_LEVEL
          assert(trace != NULL);
#endif
          if (target_gen < gen)
            trace->record_region_dependence(this, target_gen, 
                                            this, gen, target_idx, idx, dtype);
          return false;
        }
        else
          return (target_gen < gen);
      }
      bool registered_dependence = false;
      AutoLock o_lock(op_lock);
      bool prune = false;
      if (do_registration)
      {
        Event all_mapped = Event::NO_EVENT;
        prune = target->perform_registration(target_gen, this, gen,
                                                registered_dependence,
                                                outstanding_mapping_deps,
                                                outstanding_speculation_deps,
                                                all_mapped);
        if (all_mapped.exists())
          dependent_children_mapped.insert(all_mapped);
      }
      if (registered_dependence)
      {
        incoming[target] = target_gen;
        // If we registered a mapping dependence then we can verify
        verify_regions[target].insert(idx);
      }
      if (tracing)
      {
#ifdef DEBUG_HIGH_LEVEL
        assert(trace != NULL);
#endif
        trace->record_region_dependence(target, target_gen, 
                                        this, gen, target_idx, idx, dtype);
        // Unsound to prune when tracing
        prune = false;
      }
      return prune;
    }

    //--------------------------------------------------------------------------
    bool Operation::perform_registration(GenerationID our_gen, 
                                         Operation *op, GenerationID op_gen,
                                         bool &registered_dependence,
                                         unsigned &op_mapping_deps,
                                         unsigned &op_speculation_deps,
                                         Event &all_mapped)
    //--------------------------------------------------------------------------
    {
      AutoLock o_lock(op_lock);
#ifdef DEBUG_HIGH_LEVEL
      assert(our_gen <= gen); // better not be ahead of where we are now
#endif
      // If the generations match and we haven't committed yet, 
      // register an outgoing dependence
      if ((our_gen == gen) && !committed)
      {
#ifdef DEBUG_HIGH_LEVEL
        // should still have some mapping references
        // if other operations are trying to register dependences
        assert(outstanding_mapping_references > 0);
#endif
        // Check to see if we've already recorded this dependence
        std::map<Operation*,GenerationID>::const_iterator finder = 
          outgoing.find(op);
        if (finder == outgoing.end())
        {
          outgoing[op] = op_gen;
          // Record that the operation has a mapping dependence
          // on us as long as we haven't mapped
          if (!mapped)
            op_mapping_deps++;
          if (!resolved)
            op_speculation_deps++;
          // Record that we have a commit dependence on the
          // registering operation
          outstanding_commit_deps++;
          registered_dependence = true;
          // Give back the event for when all our children are mapped
          all_mapped = children_mapped;
        }
        else
        {
          // We already registered it
          registered_dependence = false;
        }
        // Cannot prune this operation from the list since it
        // is still not committed
        return false;
      }
      // We already committed so we're done and this
      // operation can be pruned from the list of users
      registered_dependence = false;
      return true;
    }

    //--------------------------------------------------------------------------
    bool Operation::is_operation_committed(GenerationID our_gen)
    //--------------------------------------------------------------------------
    {
      // If we're on an old generation then it's definitely committed
      if (our_gen < gen)
        return true;
      // Otherwise check the committed flag
      return committed;
    }

    //--------------------------------------------------------------------------
    void Operation::add_mapping_reference(GenerationID our_gen)
    //--------------------------------------------------------------------------
    {
        NAUTILUS_DEEP_DEBUG("add mapping ref\n");
      AutoLock o_lock(op_lock);
#ifdef DEBUG_HIGH_LEVEL
      assert(our_gen <= gen); // better not be ahead of where we are now
#endif
      if (our_gen == gen)
        outstanding_mapping_references++;

      NAUTILUS_DEEP_DEBUG("leave add mapping ref\n");
    }

    //--------------------------------------------------------------------------
    void Operation::remove_mapping_reference(GenerationID our_gen)
    //--------------------------------------------------------------------------
    {
      bool need_trigger = false;
      {
        AutoLock o_lock(op_lock);
#ifdef DEBUG_HIGH_LEVEL
        assert(our_gen <= gen); // better not be ahead of where we are now
#endif
        if ((our_gen == gen) && !committed)
        {
#ifdef DEBUG_HIGH_LEVEL
          assert(outstanding_mapping_references > 0);
#endif
          outstanding_mapping_references--;
          // If we've completed and we have no mapping references
          // and we have no outstanding commit dependences then 
          // we can commit this operation
          if (completed && (outstanding_mapping_references == 0) &&
              (outstanding_commit_deps == 0) && !trigger_commit_invoked)
          {
            trigger_commit_invoked = true;
            need_trigger = true;
          }
        }
        // otherwise we were already recycled and are no longer valid
      }
      if (need_trigger)
        trigger_commit();
    }

    //--------------------------------------------------------------------------
    void Operation::notify_mapping_dependence(GenerationID our_gen)
    //--------------------------------------------------------------------------
    {
      bool need_trigger = false;
      {
        AutoLock o_lock(op_lock); 
#ifdef DEBUG_HIGH_LEVEL
        assert(our_gen <= gen); // better not be ahead of where we are now
#endif
        if ((our_gen == gen) && !committed)
        {
#ifdef DEBUG_HIGH_LEVEL
          assert(outstanding_mapping_deps > 0);
#endif
          outstanding_mapping_deps--;
          if ((outstanding_mapping_deps == 0) && !trigger_mapping_invoked)
          {
            need_trigger = true;
            trigger_mapping_invoked = true;
          }
        }
      }
      if (need_trigger)
      {
        bool trigger_now = false;
        if (!dependent_children_mapped.empty())
        {
          Event wait_on = Event::merge_events(dependent_children_mapped);
          if (wait_on.exists())
          {
            DeferredMappingArgs args;
            args.hlr_id = HLR_DEFERRED_MAPPING_ID;
            args.proxy_this = this;
            args.must_epoch = must_epoch;
            args.must_epoch_gen = must_epoch_gen;
            Processor util = runtime->find_utility_group();
            util.spawn(HLR_TASK_ID, &args, sizeof(args), wait_on);
          }
          else
            trigger_now = true;
        }
        else
          trigger_now = true;
        if (trigger_now)
        {
          if (must_epoch == NULL)
            trigger_mapping();
          else
            must_epoch->notify_mapping_dependence(must_epoch_gen);
        }
      }
    }

    //--------------------------------------------------------------------------
    void Operation::notify_speculation_dependence(GenerationID our_gen)
    //--------------------------------------------------------------------------
    {
      bool need_trigger = false;
      {
        AutoLock o_lock(op_lock);
#ifdef DEBUG_HIGH_LEVEL
        assert(our_gen <= gen); // better not be ahead of where we are now
#endif
        if ((our_gen == gen) && !committed)
        {
#ifdef DEBUG_HIGH_LEVEL
          assert(!resolved);
          assert(outstanding_speculation_deps > 0);
#endif
          outstanding_speculation_deps--;
          need_trigger = !trigger_resolution_invoked &&
                          (outstanding_speculation_deps == 0);
          if (need_trigger)
            trigger_resolution_invoked = true;
        }
      }
      if (need_trigger)
        trigger_resolution();
    }

    //--------------------------------------------------------------------------
    void Operation::notify_commit_dependence(GenerationID our_gen)
    //--------------------------------------------------------------------------
    {
      bool need_trigger = false;
      {
        AutoLock o_lock(op_lock);
#ifdef DEBUG_HIGH_LEVEL
        assert(our_gen <= gen); // better not be ahead of where we are now
#endif
        if ((our_gen == gen) && !committed)
        {
#ifdef DEBUG_HIGH_LEVEL
          assert(outstanding_commit_deps > 0);
#endif
          outstanding_commit_deps--;
          if (completed && (outstanding_commit_deps == 0) &&
              (outstanding_mapping_references == 0) && !trigger_commit_invoked)
          {
            trigger_commit_invoked = true;
            need_trigger = true;
          }
        }
        // Operation was already commited and advanced
      }
      if (need_trigger)
        trigger_commit();
    } 

    //--------------------------------------------------------------------------
    void Operation::notify_regions_verified(const std::set<unsigned> &regions,
                                            GenerationID our_gen)
    //--------------------------------------------------------------------------
    {
      bool need_trigger = false;
      {
        AutoLock o_lock(op_lock);
#ifdef DEBUG_HIGH_LEVEL
        assert(our_gen <= gen); // better not be ahead of where we are now
#endif
        if ((our_gen == gen) && !committed)
        {
#ifdef DEBUG_HIGH_LEVEL
          assert(outstanding_commit_deps > 0);
#endif
          for (std::set<unsigned>::const_iterator it = regions.begin();
                it != regions.end(); it++)
          {
            unverified_regions.erase(*it);
          }
          if (completed && hardened && unverified_regions.empty()
              && !trigger_commit_invoked)
          {
            need_trigger = true;
            trigger_commit_invoked = true;
          }
        }
      }
      if (need_trigger)
        trigger_commit();
    }

    /////////////////////////////////////////////////////////////
    // Predicate Operation 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    Predicate::Impl::Impl(Runtime *rt)
      : Operation(rt)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    void Predicate::Impl::activate_predicate(void)
    //--------------------------------------------------------------------------
    {
      activate_operation();
      predicate_resolved = false;
      predicate_references = 0;
    }

    //--------------------------------------------------------------------------
    void Predicate::Impl::deactivate_predicate(void)
    //--------------------------------------------------------------------------
    {
      deactivate_operation();
#ifdef DEBUG_HIGH_LEVEL
      assert(predicate_references == 0);
#endif
      waiters.clear();
    }

    //--------------------------------------------------------------------------
    void Predicate::Impl::add_predicate_reference(void)
    //--------------------------------------------------------------------------
    {
      bool add_map_reference;
      {
        AutoLock o_lock(op_lock);
        add_map_reference = (predicate_references == 0);
        predicate_references++;
      }
      if (add_map_reference)
        add_mapping_reference(get_generation());
    }

    //--------------------------------------------------------------------------
    void Predicate::Impl::remove_predicate_reference(void)
    //--------------------------------------------------------------------------
    {
      bool need_trigger;
      bool remove_reference;
      GenerationID task_gen;
      {
        AutoLock o_lock(op_lock);
#ifdef DEBUG_HIGH_LEVEL
        assert(predicate_references > 0);
#endif
        predicate_references--;
        remove_reference = (predicate_references == 0);
        if (remove_reference)
        {
          // Get the task generation before things can be cleaned up
          task_gen = get_generation();
          need_trigger = predicate_resolved;
        }
        else
          need_trigger = false;
      }
      if (need_trigger)
        complete_execution();
      if (remove_reference)
        remove_mapping_reference(task_gen);
    }

    //--------------------------------------------------------------------------
    bool Predicate::Impl::register_waiter(PredicateWaiter *waiter,
                                          GenerationID waiter_gen, bool &value)
    //--------------------------------------------------------------------------
    {
      bool valid;
      AutoLock o_lock(op_lock);
      if (predicate_resolved)
      {
        value = predicate_value;
        valid = true;
      }
      else
      {
#ifdef DEBUG_HIGH_LEVEL
        assert(waiters.find(waiter) == waiters.end());
#endif
        waiters[waiter] = waiter_gen;
        valid = false;
      }
      return valid;
    }

    //--------------------------------------------------------------------------
    void Predicate::Impl::set_resolved_value(GenerationID pred_gen, bool value)
    //--------------------------------------------------------------------------
    {
      bool need_trigger;
      // Make a copy of the waiters since we could get cleaned up in parallel
      std::map<PredicateWaiter*,GenerationID> copy_waiters;
      {
        AutoLock o_lock(op_lock);
        if ((pred_gen == get_generation()) && !predicate_resolved)
        {
          predicate_resolved = true;
          predicate_value = value;
          copy_waiters = waiters;
          need_trigger = (predicate_references == 0);
        }
        else
          need_trigger= false;
      }
      // Notify any waiters, no need to hold the lock since waiters can't
      // be added after we set the state to resolved
      for (std::map<PredicateWaiter*,GenerationID>::const_iterator it = 
            copy_waiters.begin(); it != copy_waiters.end(); it++)
      {
        it->first->notify_predicate_value(it->second, value);
      }
      // Now see if we need to indicate we are done executing
      if (need_trigger)
        complete_execution();
    }

    /////////////////////////////////////////////////////////////
    // Speculative Operation 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    SpeculativeOp::SpeculativeOp(Runtime *rt)
      : Operation(rt)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    void SpeculativeOp::activate_speculative(void)
    //--------------------------------------------------------------------------
    {
      activate_operation();
      speculation_state = RESOLVE_TRUE_STATE;
      predicate = NULL;
      received_trigger_resolution = false;
      predicate_waiter = UserEvent::NO_USER_EVENT;
    }

    //--------------------------------------------------------------------------
    void SpeculativeOp::deactivate_speculative(void)
    //--------------------------------------------------------------------------
    {
      deactivate_operation();
    }

    //--------------------------------------------------------------------------
    void SpeculativeOp::initialize_speculation(SingleTask *ctx, bool track,
                                               Event child_event,
                                               unsigned regions,
                                               const Predicate &p)
    //--------------------------------------------------------------------------
    {
        NAUTILUS_DEEP_DEBUG("in specop:init spec (pred=%u)\n", p.const_value);
      initialize_operation(ctx, track, child_event, regions);
      NAUTILUS_DEEP_DEBUG("checking predicate\n");
      if (p == Predicate::TRUE_PRED)
      {
          NAUTILUS_DEEP_DEBUG("true pred\n");
        speculation_state = RESOLVE_TRUE_STATE;
        predicate = NULL;
      }
      else if (p == Predicate::FALSE_PRED)
      {
        speculation_state = RESOLVE_FALSE_STATE;
        predicate = NULL;
      }
      else
      {
          NAUTILUS_DEEP_DEBUG("in else\n");
        speculation_state = PENDING_MAP_STATE;
        predicate = p.impl;
        predicate->add_predicate_reference();
      }
      NAUTILUS_DEEP_DEBUG("leaving init_spec\n");
    }

    //--------------------------------------------------------------------------
    void SpeculativeOp::register_predicate_dependence(void)
    //--------------------------------------------------------------------------
    {
      if (predicate != NULL)
        register_dependence(predicate, predicate->get_generation());
    }

    //--------------------------------------------------------------------------
    bool SpeculativeOp::is_predicated(void) const
    //--------------------------------------------------------------------------
    {
      return (predicate != NULL);
    }

    //--------------------------------------------------------------------------
    bool SpeculativeOp::get_predicate_value(Processor proc)
    //--------------------------------------------------------------------------
    {
      Event wait_event = Event::NO_EVENT;
      bool result;
      {
        AutoLock o_lock(op_lock);
        if (speculation_state == RESOLVE_TRUE_STATE)
          result = true;
        else if (speculation_state == RESOLVE_FALSE_STATE)
          result = false;
        else
        {
#ifdef DEBUG_HIGH_LEVEL
          assert(predicate != NULL);
#endif
          predicate_waiter = UserEvent::create_user_event();
          wait_event = predicate_waiter;
        }
      }
      if (wait_event.exists())
      {
        if (!wait_event.has_triggered())
        {
          runtime->pre_wait(proc);
          wait_event.wait();
          runtime->post_wait(proc);
        }
        // Might be a little bit of a race here with cleanup
#ifdef DEBUG_HIGH_LEVEL
        assert((speculation_state == RESOLVE_TRUE_STATE) ||
               (speculation_state == RESOLVE_FALSE_STATE));
#endif
        if (speculation_state == RESOLVE_TRUE_STATE)
          result = true;
        else
          result = false;
      }
      return result;
    }

    //--------------------------------------------------------------------------
    void SpeculativeOp::trigger_mapping(void)
    //--------------------------------------------------------------------------
    {
      // Quick out
      if (predicate == NULL)
      {
#ifdef DEBUG_HIGH_LEVEL
        assert((speculation_state == RESOLVE_TRUE_STATE) ||
               (speculation_state == RESOLVE_FALSE_STATE));
#endif
        if (speculation_state == RESOLVE_TRUE_STATE)
          resolve_true();
        else
          resolve_false();
        return;
      }
#ifdef DEBUG_HIGH_LEVEL
      assert(predicate != NULL);
#endif
      // Register ourselves as a waiter on the predicate value
      // If the predicate hasn't resolved yet, then we can ask the
      // mapper if it would like us to speculate on the value.
      // Then take the lock and set up our state.
      bool value, speculated;
      bool valid = predicate->register_waiter(this, get_generation(), value);
      // Now that we've attempted to register ourselves with the
      // predicate we can remove the predicate reference
      predicate->remove_predicate_reference();
      if (!valid)
        speculated = speculate(value);
      // Now hold the lock and figure out what we should do
      bool continue_true = false;
      bool continue_false = false;
      bool need_resolution = false;
      bool need_trigger = false;
      {
        AutoLock o_lock(op_lock);
        switch (speculation_state)
        {
          case PENDING_MAP_STATE:
            {
              if (valid)
              {
                if (value)
                {
                  speculation_state = RESOLVE_TRUE_STATE;
                  continue_true = true;
                }
                else
                {
                  speculation_state = RESOLVE_FALSE_STATE;
                  continue_false = true;
                }
                need_resolution = received_trigger_resolution;
                need_trigger = predicate_waiter.exists();
              }
              else if (speculated)
              {
                if (value)
                {
                  speculation_state = SPECULATE_TRUE_STATE;
                  continue_true = true;
                }
                else
                {
                  speculation_state = SPECULATE_FALSE_STATE;
                  continue_false = true;
                }
              }
              // Otherwise just stay in pending map state
              break;
            }
          case RESOLVE_TRUE_STATE:
            {
              // Someone else has already resolved us to true and
              // triggered the appropriate method
              break;
            }
          case RESOLVE_FALSE_STATE:
            {
              // Someone else has already resolved us to false and
              // triggered the appropriate method
              break;
            }
          default:
            assert(false); // shouldn't be in the other states
        }
      }
      // Now do what we need to do
      if (need_trigger)
        predicate_waiter.trigger();
      if (continue_true)
        resolve_true();
      if (continue_false)
        resolve_false();
      if (need_resolution)
        resolve_speculation(); 
    }

    //--------------------------------------------------------------------------
    void SpeculativeOp::trigger_resolution(void)
    //--------------------------------------------------------------------------
    {
      // Quick out
      if (predicate == NULL)
      {
        resolve_speculation();
        return;
      }
      bool need_trigger;
      {
        AutoLock o_lock(op_lock);
#ifdef DEBUG_HIGH_LEVEL
        assert(!received_trigger_resolution);
#endif
        received_trigger_resolution = true;
        need_trigger = (speculation_state == RESOLVE_TRUE_STATE) ||
                        (speculation_state == RESOLVE_FALSE_STATE);
      }
      if (need_trigger)
        resolve_speculation();
    }

    //--------------------------------------------------------------------------
    void SpeculativeOp::notify_predicate_value(GenerationID pred_gen,bool value)
    //--------------------------------------------------------------------------
    {
      bool continue_true = false;
      bool continue_false = false;
      bool need_mispredict = false;
      bool restart = false;
      bool need_resolve = false;
      bool need_trigger = false;
      {
        AutoLock o_lock(op_lock);
#ifdef DEBUG_HIGH_LEVEL
        assert(pred_gen == get_generation());
#endif
        switch (speculation_state)
        {
          case PENDING_MAP_STATE:
            {
              if (value)
              {
                speculation_state = RESOLVE_TRUE_STATE;
                continue_true = true;
              }
              else
              {
                speculation_state = RESOLVE_FALSE_STATE;
                continue_false = true;
              }
              need_resolve = received_trigger_resolution;
              need_trigger = predicate_waiter.exists();
              break;
            }
          case SPECULATE_TRUE_STATE:
            {
              if (value) // We guessed right
              {
                speculation_state = RESOLVE_TRUE_STATE;
                need_resolve = received_trigger_resolution;
              }
              else
              {
                // We guessed wrong
                speculation_state = RESOLVE_FALSE_STATE;
                need_mispredict = true;
                restart = false;
              }
              break;
            }
          case SPECULATE_FALSE_STATE:
            {
              if (value)
              {
                speculation_state = RESOLVE_TRUE_STATE;
                need_mispredict = true;
                restart = true;
              }
              else
              {
                speculation_state = RESOLVE_FALSE_STATE;
                need_resolve = received_trigger_resolution;  
              }
              break;
            }
          default:
            assert(false); // shouldn't be in any of the other states
        }
      }
      if (need_trigger)
        predicate_waiter.trigger();
      if (continue_true)
        resolve_true();
      if (continue_false)
        resolve_false();
      if (need_mispredict)
        quash_operation(get_generation(), restart);
      if (need_resolve)
        resolve_speculation();
    }

    //--------------------------------------------------------------------------
    void SpeculativeOp::deferred_complete(void)
    //--------------------------------------------------------------------------
    {
      assert(false);
    }

    /////////////////////////////////////////////////////////////
    // Map Operation 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    MapOp::MapOp(Runtime *rt)
      : Inline(), Operation(rt)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    MapOp::MapOp(const MapOp &rhs)
      : Inline(), Operation(NULL)
    //--------------------------------------------------------------------------
    {
      // should never be called
      assert(false);
    }

    //--------------------------------------------------------------------------
    MapOp::~MapOp(void)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    MapOp& MapOp::operator=(const MapOp &rhs)
    //--------------------------------------------------------------------------
    {
      // should never be called
      assert(false);
      return *this;
    }

    //--------------------------------------------------------------------------
    PhysicalRegion MapOp::initialize(SingleTask *ctx, 
                                     const InlineLauncher &launcher,
                                     bool check_privileges)
    //--------------------------------------------------------------------------
    {
      initialize_operation(ctx, true/*track*/, Event::NO_EVENT);
      parent_task = ctx;
      if (launcher.requirement.privilege_fields.empty())
      {
        log_task(LEVEL_WARNING,"WARNING: REGION REQUIREMENT OF INLINE MAPPING "
                               "IN TASK %s (ID %lld) HAS NO PRIVILEGE "
                               "FIELDS! DID YOU FORGET THEM?!?",
                               parent_ctx->variants->name, 
                               parent_ctx->get_unique_task_id());
      }
      requirement.copy_without_mapping_info(launcher.requirement);
      requirement.initialize_mapping_fields();
      if (parent_ctx->has_simultaneous_coherence())
        parent_ctx->check_simultaneous_restricted(requirement);
      map_id = launcher.map_id;
      tag = launcher.tag;
      parent_task = ctx;
      termination_event = UserEvent::create_user_event();
      region = PhysicalRegion(legion_new<PhysicalRegion::Impl>(requirement,
                              completion_event, true/*mapped*/, ctx, 
                              map_id, tag, false/*leaf*/, runtime));
      if (check_privileges)
        check_privilege();
      initialize_privilege_path(privilege_path, requirement);
      initialize_mapping_path(mapping_path, requirement, requirement.region);
#ifdef LEGION_LOGGING
      LegionLogging::log_mapping_operation(
          parent_ctx->get_executing_processor(),
          parent_ctx->get_unique_op_id(),
          unique_op_id);
      LegionLogging::log_logical_requirement(
                                         parent_ctx->get_executing_processor(),
                                         unique_op_id, 0/*idx*/, true/*region*/,
                                         requirement.region.index_space.id,
                                         requirement.region.field_space.id,
                                         requirement.region.tree_id,
                                         requirement.privilege,
                                         requirement.prop,
                                         requirement.redop,
                                         requirement.privilege_fields);
#endif
#ifdef LEGION_PROF
      LegionProf::register_map(unique_op_id, parent_ctx->get_unique_task_id());
#endif
#ifdef LEGION_SPY
      LegionSpy::log_mapping_operation(parent_ctx->get_unique_task_id(),
                                       unique_op_id);
      LegionSpy::log_logical_requirement(unique_op_id,0/*index*/,
                                         true/*region*/,
                                         requirement.region.index_space.id,
                                         requirement.region.field_space.id,
                                         requirement.region.tree_id,
                                         requirement.privilege,
                                         requirement.prop,
                                         requirement.redop);
      LegionSpy::log_requirement_fields(unique_op_id, 0/*index*/,
                                        requirement.privilege_fields);
#endif
      return region;
    }

    //--------------------------------------------------------------------------
    PhysicalRegion MapOp::initialize(SingleTask *ctx,
                                     const RegionRequirement &req,
                                     MapperID id, MappingTagID t,
                                     bool check_privileges)
    //--------------------------------------------------------------------------
    {
      initialize_operation(ctx, true/*track*/, Event::NO_EVENT);
      parent_task = ctx;
      requirement = req;
      if (requirement.privilege_fields.empty())
      {
        log_task(LEVEL_WARNING,"WARNING: REGION REQUIREMENT OF INLINE MAPPING "
                               "IN TASK %s (ID %lld) HAS NO PRIVILEGE "
                               "FIELDS! DID YOU FORGET THEM?!?",
                               parent_ctx->variants->name, 
                               parent_ctx->get_unique_task_id());
      }
      requirement.copy_without_mapping_info(req);
      requirement.initialize_mapping_fields();
      if (parent_ctx->has_simultaneous_coherence())
        parent_ctx->check_simultaneous_restricted(requirement);
      map_id = id;
      tag = t;
      parent_task = ctx;
      termination_event = UserEvent::create_user_event();
      region = PhysicalRegion(legion_new<PhysicalRegion::Impl>(requirement,
                              completion_event, true/*mapped*/, ctx, 
                              map_id, tag, false/*leaf*/, runtime));
      if (check_privileges)
        check_privilege();
      initialize_privilege_path(privilege_path, requirement);
      initialize_mapping_path(mapping_path, requirement, requirement.region);
#ifdef LEGION_LOGGING
      LegionLogging::log_mapping_operation(
          parent_ctx->get_executing_processor(),
          parent_ctx->get_unique_op_id(),
          unique_op_id);
      LegionLogging::log_logical_requirement(
                                         parent_ctx->get_executing_processor(),
                                         unique_op_id, 0/*idx*/, true/*region*/,
                                         requirement.region.index_space.id,
                                         requirement.region.field_space.id,
                                         requirement.region.tree_id,
                                         requirement.privilege,
                                         requirement.prop,
                                         requirement.redop,
                                         requirement.privilege_fields);
#endif
#ifdef LEGION_PROF
      LegionProf::register_map(unique_op_id, parent_ctx->get_unique_task_id());
#endif
#ifdef LEGION_SPY
      LegionSpy::log_mapping_operation(parent_ctx->get_unique_task_id(),
                                       unique_op_id);
      LegionSpy::log_logical_requirement(unique_op_id,0/*index*/,
                                         true/*region*/,
                                         requirement.region.index_space.id,
                                         requirement.region.field_space.id,
                                         requirement.region.tree_id,
                                         requirement.privilege,
                                         requirement.prop,
                                         requirement.redop);
      LegionSpy::log_requirement_fields(unique_op_id, 0/*index*/,
                                        requirement.privilege_fields);
#endif
      return region;
    }

    //--------------------------------------------------------------------------
    void MapOp::initialize(SingleTask *ctx, const PhysicalRegion &reg)
    //--------------------------------------------------------------------------
    {
      initialize_operation(ctx, true/*track*/, Event::NO_EVENT);
      parent_task = ctx;
      requirement.copy_without_mapping_info(reg.impl->get_requirement());
      requirement.initialize_mapping_fields();
      map_id = reg.impl->map_id;
      tag = reg.impl->tag;
      parent_task = ctx;
      termination_event = UserEvent::create_user_event();
      region = reg;
      region.impl->remap_region(completion_event);
      // We're only really remapping it if it already had a physical
      // instance that we can use to make a valid value
      remap_region = region.impl->get_reference().has_ref();
      // No need to check the privileges here since we know that we have
      // them from the first time that we made this physical region
      initialize_privilege_path(privilege_path, requirement);
      initialize_mapping_path(mapping_path, requirement, requirement.region);
#ifdef LEGION_LOGGING
      LegionLogging::log_mapping_operation(
          parent_ctx->get_executing_processor(),
          parent_ctx->get_unique_op_id(),
          unique_op_id);
      LegionLogging::log_logical_requirement(
                                         parent_ctx->get_executing_processor(),
                                         unique_op_id, 0/*idx*/, true/*region*/,
                                         requirement.region.index_space.id,
                                         requirement.region.field_space.id,
                                         requirement.region.tree_id,
                                         requirement.privilege,
                                         requirement.prop,
                                         requirement.redop,
                                         requirement.privilege_fields);
#endif
#ifdef LEGION_PROF
      LegionProf::register_map(unique_op_id, parent_ctx->get_unique_task_id());
#endif
#ifdef LEGION_SPY
      LegionSpy::log_mapping_operation(parent_ctx->get_unique_task_id(), 
                                       unique_op_id);
      LegionSpy::log_logical_requirement(unique_op_id,0/*index*/,
                                         true/*region*/,
                                         requirement.region.index_space.id,
                                         requirement.region.field_space.id,
                                         requirement.region.tree_id,
                                         requirement.privilege,
                                         requirement.prop,
                                         requirement.redop);
      LegionSpy::log_requirement_fields(unique_op_id, 0/*index*/,
                                        requirement.privilege_fields);
#endif
    }

    //--------------------------------------------------------------------------
    void MapOp::activate(void)
    //--------------------------------------------------------------------------
    {
      activate_operation();
      parent_ctx = NULL;
      remap_region = false;
    }

    //--------------------------------------------------------------------------
    void MapOp::deactivate(void)
    //--------------------------------------------------------------------------
    {
      deactivate_operation();
      // Remove our reference to the region
      region = PhysicalRegion();
      privilege_path = RegionTreePath();
      mapping_path = RegionTreePath();
      // Now return this operation to the queue
      runtime->free_map_op(this);
    } 

    //--------------------------------------------------------------------------
    const char* MapOp::get_logging_name(void)
    //--------------------------------------------------------------------------
    {
      return "Mapping";
    }

    //--------------------------------------------------------------------------
    void MapOp::trigger_dependence_analysis(void)
    //--------------------------------------------------------------------------
    {
#ifdef LEGION_LOGGING
      LegionLogging::log_timing_event(Machine::get_executing_processor(),
                                      unique_op_id, BEGIN_DEPENDENCE_ANALYSIS); 
#endif
#ifdef LEGION_PROF
      LegionProf::register_event(unique_op_id, PROF_BEGIN_DEP_ANALYSIS);
#endif
      begin_dependence_analysis();
      runtime->forest->perform_dependence_analysis(parent_ctx->get_context(),
                                                   this, 0/*idx*/, requirement,
                                                   privilege_path);
      end_dependence_analysis();
#ifdef LEGION_LOGGING
      LegionLogging::log_timing_event(Machine::get_executing_processor(),
                                      unique_op_id, END_DEPENDENCE_ANALYSIS);
#endif
#ifdef LEGION_PROF
      LegionProf::register_event(unique_op_id, PROF_END_DEP_ANALYSIS);
#endif
    }

    //--------------------------------------------------------------------------
    bool MapOp::trigger_execution(void)
    //--------------------------------------------------------------------------
    {
#ifdef LEGION_LOGGING
      LegionLogging::log_timing_event(Machine::get_executing_processor(),
                                      unique_op_id, BEGIN_MAPPING);
#endif
#ifdef LEGION_PROF
      LegionProf::register_event(unique_op_id, PROF_BEGIN_MAP_ANALYSIS);
#endif
      RegionTreeContext physical_ctx = 
        parent_ctx->find_enclosing_physical_context(requirement.parent);
      Processor local_proc = parent_ctx->get_executing_processor();
      // If we haven't already premapped the path, then do so now
      if (!requirement.premapped)
      {
        requirement.premapped = runtime->forest->premap_physical_region(
                  physical_ctx, privilege_path, requirement, 
                  this, parent_ctx, local_proc
#ifdef DEBUG_HIGH_LEVEL
                  , 0/*idx*/, get_logging_name(), unique_op_id
#endif
                  );
      }
      // If we couldn't premap, then we need to try again later
      if (!requirement.premapped)
        return false;
      MappingRef map_ref;
      bool notify = false;
      if (!remap_region)
      {
        // Now ask the mapper how to map this inline mapping operation 
        notify = runtime->invoke_mapper_map_inline(local_proc, this);
        // Do the mapping and see if it succeeded 
        map_ref = runtime->forest->map_physical_region(physical_ctx,
                                                       mapping_path,
                                                       requirement,
                                                       0/*idx*/,
                                                       this,
                                                       local_proc,
                                                       local_proc
#ifdef DEBUG_HIGH_LEVEL
                                                       , get_logging_name()
                                                       , unique_op_id
#endif
                                                       );
        if (!map_ref.has_ref())
        {
          requirement.mapping_failed = true;
          requirement.selected_memory = Memory::NO_MEMORY;
          // Tell the mapper that we failed to map
          runtime->invoke_mapper_failed_mapping(local_proc, this);
          return false;
        }
      }
      else
      {
        // We're remapping the region so we don't actually
        // need to ask the mapper about anything when doing the remapping
        map_ref = runtime->forest->remap_physical_region(physical_ctx,
                                                         requirement,
                                                         0/*idx*/,
                                                 region.impl->get_reference()
#ifdef DEBUG_HIGH_LEVEL
                                                         , get_logging_name()
                                                         , unique_op_id
#endif
                                                         );
#ifdef DEBUG_HIGH_LEVEL
        assert(map_ref.has_ref());
#endif
      }
      InstanceRef result = runtime->forest->register_physical_region(
                                                                physical_ctx,
                                                                map_ref,
                                                                requirement,
                                                                0/*idx*/,
                                                                this,
                                                                local_proc,
                                                            termination_event
#ifdef DEBUG_HIGH_LEVEL
                                                          , get_logging_name()
                                                          , unique_op_id
                                                          , mapping_path
#endif
                                                            );
#ifdef DEBUG_HIGH_LEVEL
      assert(result.has_ref());
#endif
      // We succeeded in mapping, so set up our physical region with
      // the reference information.  Note that the physical region
      // becomes responsible for triggering the termination event
      // when it unmaps so we can recycle this mapping operation 
      // immediately.
      if (notify)
      {
        requirement.mapping_failed = false;
        requirement.selected_memory = result.get_memory();
        runtime->invoke_mapper_notify_result(local_proc, this);
      }
#ifdef LEGION_LOGGING
      LegionLogging::log_timing_event(Machine::get_executing_processor(),
                                      unique_op_id, END_MAPPING);
      LegionLogging::log_operation_events(
          Machine::get_executing_processor(),
          unique_op_id, Event::NO_EVENT, result.get_ready_event());
      LegionLogging::log_event_dependence(
          Machine::get_executing_processor(),
          termination_event, parent_ctx->get_task_completion());
      LegionLogging::log_event_dependence(
          Machine::get_executing_processor(),
          result.get_ready_event(),
          completion_event);
      LegionLogging::log_event_dependence(
          Machine::get_executing_processor(),
          completion_event,
          termination_event);
      LegionLogging::log_physical_user(
          Machine::get_executing_processor(),
          result.get_handle().get_view()->get_manager()->get_instance(),
          unique_op_id, 0/*idx*/);
#endif
#ifdef LEGION_PROF
      LegionProf::register_event(unique_op_id, PROF_END_MAP_ANALYSIS);
#endif
#ifdef LEGION_SPY
      // Log an implicit dependence on the parent's start event
      LegionSpy::log_implicit_dependence(parent_ctx->get_start_event(),
                                         result.get_ready_event());
      LegionSpy::log_op_events(unique_op_id, result.get_ready_event(),
                                termination_event);
      // Log an implicit dependence on the parent's term event
      LegionSpy::log_implicit_dependence(termination_event, 
                                         parent_ctx->get_task_completion());
      LegionSpy::log_op_user(unique_op_id, 0/*idx*/, 
          result.get_handle().get_view()->get_manager()->get_instance().id);
#endif
      // Have to do this before triggering the mapped event
      region.impl->reset_reference(result, termination_event);
      // Now we can trigger the mapping event and indicate
      // to all our mapping dependences that we are mapped.
      complete_mapping();
      // Note that completing mapping and execution should
      // be enough to trigger the completion operation call
      // Trigger an early commit of this operation
      bool commit_early = request_early_commit();
      // Note that a mapping operation terminates as soon as it
      // is done mapping reflecting that after this happens, information
      // has flowed back out into the application task's execution.
      // Therefore mapping operations cannot be restarted because we
      // cannot track how the application task uses their data.
      // This means that any attempts to restart an inline mapping
      // will result in the entire task needing to be restarted.
      complete_execution();
      if (commit_early)
        trigger_commit();
      // return true since we succeeded
      return true;
    }

    //--------------------------------------------------------------------------
    Mappable::MappableKind MapOp::get_mappable_kind(void) const
    //--------------------------------------------------------------------------
    {
      return INLINE_MAPPABLE;
    }

    //--------------------------------------------------------------------------
    Task* MapOp::as_mappable_task(void) const
    //--------------------------------------------------------------------------
    {
      return NULL;
    }

    //--------------------------------------------------------------------------
    Copy* MapOp::as_mappable_copy(void) const
    //--------------------------------------------------------------------------
    {
      return NULL;
    }

    //--------------------------------------------------------------------------
    Inline* MapOp::as_mappable_inline(void) const
    //--------------------------------------------------------------------------
    {
      MapOp *proxy_this = const_cast<MapOp*>(this);
      return proxy_this;
    }

    //--------------------------------------------------------------------------
    Acquire* MapOp::as_mappable_acquire(void) const
    //--------------------------------------------------------------------------
    {
      return NULL;
    }

    //--------------------------------------------------------------------------
    Release* MapOp::as_mappable_release(void) const
    //--------------------------------------------------------------------------
    {
      return NULL;
    }

    //--------------------------------------------------------------------------
    UniqueID MapOp::get_unique_mappable_id(void) const
    //--------------------------------------------------------------------------
    {
      return unique_op_id;
    } 

    //--------------------------------------------------------------------------
    void MapOp::check_privilege(void)
    //--------------------------------------------------------------------------
    { 
      if ((requirement.handle_type == PART_PROJECTION) || 
          (requirement.handle_type == REG_PROJECTION))
      {
        log_region(LEVEL_ERROR,"Projection region requirements are not "
                               "permitted for inline mappings (in task %s)",
                               parent_ctx->variants->name);
#ifdef DEBUG_HIGH_LEVEL
        assert(false);
#endif
        exit(ERROR_BAD_PROJECTION_USE);
      }
      FieldID bad_field;
      LegionErrorType et = runtime->verify_requirement(requirement, bad_field);
      // If that worked, then check the privileges with the parent context
      if (et == NO_ERROR)
        et = parent_ctx->check_privilege(requirement, bad_field);
      switch (et)
      {
        case NO_ERROR:
          break;
        case ERROR_INVALID_REGION_HANDLE:
          {
            log_region(LEVEL_ERROR,"Requirest for invalid region handle "
                                   "(" IDFMT ",%d,%d) for inline mapping "
                                   "(ID %lld)",
                                   requirement.region.index_space.id, 
                                   requirement.region.field_space.id, 
                                   requirement.region.tree_id, 
                                   unique_op_id);
#ifdef DEBUG_HIGH_LEVEL
            assert(false);
#endif
            exit(ERROR_INVALID_REGION_HANDLE);
          }
        case ERROR_FIELD_SPACE_FIELD_MISMATCH:
          {
            FieldSpace sp = (requirement.handle_type == SINGULAR) || 
                            (requirement.handle_type == REG_PROJECTION)
                             ? requirement.region.field_space : 
                               requirement.partition.field_space;
            log_region(LEVEL_ERROR,"Field %d is not a valid field of field "
                                   "space %d for inline mapping (ID %lld)",
                                   bad_field, sp.id, unique_op_id);
#ifdef DEBUG_HIGH_LEVEL
            assert(false);
#endif
            exit(ERROR_FIELD_SPACE_FIELD_MISMATCH);
          }
        case ERROR_INVALID_INSTANCE_FIELD:
          {
            log_region(LEVEL_ERROR,"Instance field %d is not one of the "
                                   "privilege fields for inline mapping "
                                   "(ID %lld)",
                                    bad_field, unique_op_id);
#ifdef DEBUG_HIGH_LEVEL
            assert(false);
#endif
            exit(ERROR_INVALID_INSTANCE_FIELD);
          }
        case ERROR_DUPLICATE_INSTANCE_FIELD:
          {
            log_region(LEVEL_ERROR, "Instance field %d is a duplicate for "
                                    "inline mapping (ID %lld)",
                                    bad_field, unique_op_id);
#ifdef DEBUG_HIGH_LEVEL
            assert(false);
#endif
            exit(ERROR_DUPLICATE_INSTANCE_FIELD);
          }
        case ERROR_BAD_PARENT_REGION:
          {
            log_region(LEVEL_ERROR,"Parent task %s (ID %lld) of inline mapping "
                                   "(ID %lld) does not have a region "
                                   "requirement for region (" IDFMT ",%x,%x) "
                                   "as a parent of region requirement",
                                   parent_ctx->variants->name, 
                                   parent_ctx->get_unique_task_id(),
                                   unique_op_id, 
                                   requirement.region.index_space.id,
                                   requirement.region.field_space.id, 
                                   requirement.region.tree_id);
#ifdef DEBUG_HIGH_LEVEL
            assert(false);
#endif
            exit(ERROR_BAD_PARENT_REGION);
          }
        case ERROR_BAD_REGION_PATH:
          {
            log_region(LEVEL_ERROR,"Region (" IDFMT ",%x,%x) is not a "
                                   "sub-region of parent region (" IDFMT 
                                   ",%x,%x) for region requirement of inline "
                                   "mapping (ID %lld)",
                                   requirement.region.index_space.id,
                                   requirement.region.field_space.id, 
                                   requirement.region.tree_id,
                                   requirement.parent.index_space.id,
                                   requirement.parent.field_space.id,
                                   requirement.parent.tree_id,
                                   unique_op_id);
#ifdef DEBUG_HIGH_LEVEL
            assert(false);
#endif
            exit(ERROR_BAD_REGION_PATH);
          }
        case ERROR_BAD_REGION_TYPE:
          {
            log_region(LEVEL_ERROR,"Region requirement of inline mapping "
                                   "(ID %lld) cannot find privileges for field "
                                   "%d in parent task",
                                   unique_op_id, bad_field);
#ifdef DEBUG_HIGH_LEVEL
            assert(false);
#endif
            exit(ERROR_BAD_REGION_TYPE);
          }
        case ERROR_BAD_REGION_PRIVILEGES:
          {
            log_region(LEVEL_ERROR,"Privileges %x for region (" IDFMT 
                                   ",%x,%x) are not a subset of privileges "
                                   "of parent task's privileges for region "
                                   "requirement of inline mapping (ID %lld)",
                                   requirement.privilege, 
                                   requirement.region.index_space.id,
                                   requirement.region.field_space.id, 
                                   requirement.region.tree_id, 
                                   unique_op_id);
#ifdef DEBUG_HIGH_LEVEL
            assert(false);
#endif
            exit(ERROR_BAD_REGION_PRIVILEGES);
          }
        // this should never happen with an inline mapping
        case ERROR_NON_DISJOINT_PARTITION: 
        default:
          assert(false); // Should never happen
      }
    }

    /////////////////////////////////////////////////////////////
    // Copy Operation 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    CopyOp::CopyOp(Runtime *rt)
      : Copy(), SpeculativeOp(rt)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    CopyOp::CopyOp(const CopyOp &rhs)
      : Copy(), SpeculativeOp(NULL)
    //--------------------------------------------------------------------------
    {
      // should never be called
      assert(false);
    }

    //--------------------------------------------------------------------------
    CopyOp::~CopyOp(void)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    CopyOp& CopyOp::operator=(const CopyOp &rhs)
    //--------------------------------------------------------------------------
    {
      // should never be called
      assert(false);
      return *this;
    }

    //--------------------------------------------------------------------------
    void CopyOp::initialize(SingleTask *ctx,
                            const CopyLauncher &launcher, bool check_privileges)
    //--------------------------------------------------------------------------
    {
      parent_ctx = ctx;
      parent_task = ctx;
      initialize_speculation(ctx, true/*track*/, Event::NO_EVENT, 
                             launcher.src_requirements.size() + 
                               launcher.dst_requirements.size(), 
                             launcher.predicate);
      src_requirements.resize(launcher.src_requirements.size());
      dst_requirements.resize(launcher.dst_requirements.size());
      for (unsigned idx = 0; idx < src_requirements.size(); idx++)
      {
        if (launcher.src_requirements[idx].privilege_fields.empty())
        {
          log_task(LEVEL_WARNING,"WARNING: SOURCE REGION REQUIREMENT %d OF "
                                 "COPY (ID %lld) IN TASK %s (ID %lld) HAS NO "
                                 "PRIVILEGE FIELDS! DID YOU FORGET THEM?!?",
                                 idx, get_unique_op_id(),
                                 parent_ctx->variants->name, 
                                 parent_ctx->get_unique_task_id());
        }
        src_requirements[idx].copy_without_mapping_info(
            launcher.src_requirements[idx]);
        src_requirements[idx].initialize_mapping_fields();
        src_requirements[idx].flags |= NO_ACCESS_FLAG;
      }
      for (unsigned idx = 0; idx < dst_requirements.size(); idx++)
      {
        if (launcher.src_requirements[idx].privilege_fields.empty())
        {
          log_task(LEVEL_WARNING,"WARNING: DESTINATION REGION REQUIREMENT %d OF"
                                 " COPY (ID %lld) IN TASK %s (ID %lld) HAS NO "
                                 "PRIVILEGE FIELDS! DID YOU FORGET THEM?!?",
                                 idx, get_unique_op_id(),
                                 parent_ctx->variants->name, 
                                 parent_ctx->get_unique_task_id());
        }
        dst_requirements[idx].copy_without_mapping_info(
            launcher.dst_requirements[idx]);
        dst_requirements[idx].initialize_mapping_fields();
        dst_requirements[idx].flags |= NO_ACCESS_FLAG;
      }
      if (parent_ctx->has_simultaneous_coherence())
      {
        parent_ctx->check_simultaneous_restricted(src_requirements);
        parent_ctx->check_simultaneous_restricted(dst_requirements);
      }
      grants = launcher.grants;
      // Register ourselves with all the grants
      for (unsigned idx = 0; idx < grants.size(); idx++)
        grants[idx].impl->register_operation(completion_event);
      wait_barriers = launcher.wait_barriers;
      for (std::vector<PhaseBarrier>::const_iterator it = 
            launcher.arrive_barriers.begin(); it != 
            launcher.arrive_barriers.end(); it++)
      {
        // TODO: Put this back in once Sean fixes barriers
#if 0
        arrive_barriers.push_back(
            PhaseBarrier(it->phase_barrier.alter_arrival_count(1),
                         it->participants));
#else
        arrive_barriers.push_back(*it);
#endif
#ifdef LEGION_LOGGING
        LegionLogging::log_event_dependence(
            Machine::get_executing_processor(),
            it->phase_barrier, arrive_barriers.back().phase_barrier);
#endif
#ifdef LEGION_SPY
        LegionSpy::log_event_dependence(it->phase_barrier,
                                arrive_barriers.back().phase_barrier);
#endif
      }
      map_id = launcher.map_id;
      tag = launcher.tag;
      if (check_privileges)
      {
        if (src_requirements.size() != dst_requirements.size())
        {
          log_run(LEVEL_ERROR,"Number of source requirements (%ld) does not "
                              "match number of destination requirements (%ld) "
                              "for copy operation (ID %lld) with parent "
                              "task %s (ID %lld)",
                              src_requirements.size(), dst_requirements.size(),
                              get_unique_copy_id(), parent_ctx->variants->name,
                              parent_ctx->get_unique_task_id());
#ifdef DEBUG_HIGH_LEVEL
          assert(false);
#endif
          exit(ERROR_COPY_REQUIREMENTS_MISMATCH);
        }
        for (unsigned idx = 0; idx < src_requirements.size(); idx++)
        {
          if (src_requirements[idx].privilege_fields.size() != 
              src_requirements[idx].instance_fields.size())
          {
            log_run(LEVEL_ERROR,"Copy source requirement %d for copy operation "
                                "(ID %lld) in parent task %s (ID %lld) has %ld "
                                "privilege fields and %ld instance fields.  "
                                "Copy requirements must have exactly the same "
                                "number of privilege and instance fields.",
                                idx, get_unique_copy_id(), 
                                parent_ctx->variants->name,
                                parent_ctx->get_unique_task_id(),
                                src_requirements[idx].privilege_fields.size(),
                                src_requirements[idx].instance_fields.size());
#ifdef DEBUG_HIGH_LEVEL
            assert(false);
#endif
            exit(ERROR_INVALID_COPY_FIELDS_SIZE);
          }
          if (!IS_READ_ONLY(src_requirements[idx]))
          {
            log_run(LEVEL_ERROR,"Copy source requirement %d for copy operation "
                                "(ID %lld) in parent task %s (ID %lld) must "
                                "be requested with a read-only privilege.",
                                idx, get_unique_copy_id(),
                                parent_ctx->variants->name,
                                parent_ctx->get_unique_task_id());
#ifdef DEBUG_HIGH_LEVEL
            assert(false);
#endif
            exit(ERROR_INVALID_COPY_PRIVILEGE);
          }
          check_copy_privilege(src_requirements[idx], idx, true/*src*/);
        }
        for (unsigned idx = 0; idx < dst_requirements.size(); idx++)
        {
          if (dst_requirements[idx].privilege_fields.size() != 
              dst_requirements[idx].instance_fields.size())
          {
            log_run(LEVEL_ERROR,"Copy destination requirement %d for copy "
                                "operation (ID %lld) in parent task %s "
                                "(ID %lld) has %ld privilege fields and %ld "
                                "instance fields.  Copy requirements must "
                                "have exactly the same number of  privilege "
                                "and instance fields.", idx, 
                                get_unique_copy_id(), 
                                parent_ctx->variants->name,
                                parent_ctx->get_unique_task_id(),
                                dst_requirements[idx].privilege_fields.size(),
                                dst_requirements[idx].instance_fields.size());
#ifdef DEBUG_HIGH_LEVEL
            assert(false);
#endif
            exit(ERROR_INVALID_COPY_FIELDS_SIZE);
          }
          if (!IS_WRITE(dst_requirements[idx]))
          {
            log_run(LEVEL_ERROR,"Copy destination requirement %d for copy "
                                "operation (ID %lld) in parent task %s "
                                "(ID %lld) must be requested with a "
                                "read-write or write-discard privilege.",
                                idx, get_unique_copy_id(),
                                parent_ctx->variants->name,
                                parent_ctx->get_unique_task_id());
#ifdef DEBUG_HIGH_LEVEL
            assert(false);
#endif
            exit(ERROR_INVALID_COPY_PRIVILEGE);
          }
          check_copy_privilege(dst_requirements[idx], idx, false/*src*/);
        }
        for (unsigned idx = 0; idx < src_requirements.size(); idx++)
        {
          IndexSpace src_space = src_requirements[idx].region.get_index_space();
          IndexSpace dst_space = dst_requirements[idx].region.get_index_space();
          if (!runtime->forest->are_compatible(src_space, dst_space))
          {
            log_run(LEVEL_ERROR,"Copy launcher index space mismatch at index "
                                "%d of cross-region copy (ID %lld) in task %s "
                                "(ID %lld). Source requirement with index "
                                "space " IDFMT " and destination requirement "
                                "with index space " IDFMT " do not have the "
                                "same number of dimensions or the same number "
                                "of elements in their element masks.",
                                idx, get_unique_copy_id(),
                                parent_ctx->variants->name, 
                                parent_ctx->get_unique_task_id(),
                                src_space.id, dst_space.id);
#ifdef DEBUG_HIGH_LEVEL
            assert(false);
#endif
            exit(ERROR_COPY_SPACE_MISMATCH);
          }
          else if (!runtime->forest->is_dominated(src_space, dst_space))
          {
            log_run(LEVEL_ERROR,"Destination index space " IDFMT " for "
                                "requirement %d of cross-region copy "
                                "(ID %lld) in task %s (ID %lld) is not "
                                "a sub-region of the source index space " 
                                IDFMT ".",
                                dst_space.id, idx, get_unique_copy_id(),
                                parent_ctx->variants->name,
                                parent_ctx->get_unique_task_id(),
                                src_space.id);
#ifdef DEBUG_HIGH_LEVEL
            assert(false);
#endif
            exit(ERROR_COPY_SPACE_MISMATCH);
          }
        }
      }
      // Initialize the privilege and mapping paths for all of the
      // region requirements that we have
      src_privilege_paths.resize(src_requirements.size());
      src_mapping_paths.resize(src_requirements.size());
      for (unsigned idx = 0; idx < src_requirements.size(); idx++)
      {
        initialize_privilege_path(src_privilege_paths[idx],
                                  src_requirements[idx]);
        initialize_mapping_path(src_mapping_paths[idx],
                                src_requirements[idx],
                                src_requirements[idx].region);
      }
      dst_privilege_paths.resize(dst_requirements.size());
      dst_mapping_paths.resize(dst_requirements.size());
      for (unsigned idx = 0; idx < dst_requirements.size(); idx++)
      {
        initialize_privilege_path(dst_privilege_paths[idx],
                                  dst_requirements[idx]);
        initialize_mapping_path(dst_mapping_paths[idx],
                                dst_requirements[idx],
                                dst_requirements[idx].region);
      }
#ifdef LEGION_LOGGING
      LegionLogging::log_copy_operation(parent_ctx->get_executing_processor(),
                                        parent_ctx->get_unique_op_id(),
                                        unique_op_id);
      for (unsigned idx = 0; idx < src_requirements.size(); idx++)
      {
        const RegionRequirement &req = src_requirements[idx];
        LegionLogging::log_logical_requirement(
                                        parent_ctx->get_executing_processor(),
                                        unique_op_id, idx, true/*region*/,
                                        req.region.index_space.id,
                                        req.region.field_space.id,
                                        req.region.tree_id,
                                        req.privilege,
                                        req.prop,
                                        req.redop,
                                        req.privilege_fields);
      }
      for (unsigned idx = 0; idx < dst_requirements.size(); idx++)
      {
        const RegionRequirement &req = dst_requirements[idx];
        LegionLogging::log_logical_requirement(
                                        parent_ctx->get_executing_processor(),
                                        unique_op_id, 
                                        src_requirements.size()+idx, 
                                        true/*region*/,
                                        req.region.index_space.id,
                                        req.region.field_space.id,
                                        req.region.tree_id,
                                        req.privilege,
                                        req.prop,
                                        req.redop,
                                        req.privilege_fields);
      }
#endif
#ifdef LEGION_PROF
      LegionProf::register_copy(unique_op_id,
                                parent_ctx->get_unique_task_id());
#endif
#ifdef LEGION_SPY
      LegionSpy::log_copy_operation(parent_ctx->get_unique_task_id(),
                                    unique_op_id);
      for (unsigned idx = 0; idx < src_requirements.size(); idx++)
      {
        const RegionRequirement &req = src_requirements[idx];
        LegionSpy::log_logical_requirement(unique_op_id, idx, true/*region*/,
                                           req.region.index_space.id,
                                           req.region.field_space.id,
                                           req.region.tree_id,
                                           req.privilege,
                                           req.prop, req.redop);
        LegionSpy::log_requirement_fields(unique_op_id, idx, 
                                          req.privilege_fields);
      }
      for (unsigned idx = 0; idx < dst_requirements.size(); idx++)
      {
        const RegionRequirement &req = dst_requirements[idx];
        LegionSpy::log_logical_requirement(unique_op_id, 
                                           src_requirements.size()+idx, 
                                           true/*region*/,
                                           req.region.index_space.id,
                                           req.region.field_space.id,
                                           req.region.tree_id,
                                           req.privilege,
                                           req.prop, req.redop);
        LegionSpy::log_requirement_fields(unique_op_id, 
                                          src_requirements.size()+idx, 
                                          req.privilege_fields);
      }
#endif
    }

    //--------------------------------------------------------------------------
    void CopyOp::activate(void)
    //--------------------------------------------------------------------------
    {
      activate_speculative();
    }

    //--------------------------------------------------------------------------
    void CopyOp::deactivate(void)
    //--------------------------------------------------------------------------
    {
      deactivate_speculative();
      // Clear out our region tree state
      src_requirements.clear();
      dst_requirements.clear();
      grants.clear();
      wait_barriers.clear();
      arrive_barriers.clear();
      src_privilege_paths.clear();
      dst_privilege_paths.clear();
      src_mapping_paths.clear();
      dst_mapping_paths.clear();
      // Return this operation to the runtime
      runtime->free_copy_op(this);
    }

    //--------------------------------------------------------------------------
    const char* CopyOp::get_logging_name(void)
    //--------------------------------------------------------------------------
    {
      return "Copy";
    }

    //--------------------------------------------------------------------------
    void CopyOp::trigger_dependence_analysis(void)
    //--------------------------------------------------------------------------
    {
#ifdef LEGION_LOGGING
      LegionLogging::log_timing_event(Machine::get_executing_processor(),
                                      unique_op_id, BEGIN_DEPENDENCE_ANALYSIS);
#endif
#ifdef LEGION_PROF
      LegionProf::register_event(unique_op_id, PROF_BEGIN_DEP_ANALYSIS);
#endif
      begin_dependence_analysis();
      // Register a dependence on our predicate
      register_predicate_dependence();
      for (unsigned idx = 0; idx < src_requirements.size(); idx++)
      {
        runtime->forest->perform_dependence_analysis(parent_ctx->get_context(),
                                                     this, idx, 
                                                     src_requirements[idx],
                                                     src_privilege_paths[idx]);
      }
      for (unsigned idx = 0; idx < dst_requirements.size(); idx++)
      {
        unsigned index = src_requirements.size()+idx;
        runtime->forest->perform_dependence_analysis(parent_ctx->get_context(),
                                                     this, index, 
                                                     dst_requirements[idx],
                                                     dst_privilege_paths[idx]);
      }
      end_dependence_analysis();
#ifdef LEGION_LOGGING
      LegionLogging::log_timing_event(Machine::get_executing_processor(),
                                      unique_op_id, END_DEPENDENCE_ANALYSIS);
#endif
#ifdef LEGION_PROF
      LegionProf::register_event(unique_op_id, PROF_END_DEP_ANALYSIS);
#endif
    }

    //--------------------------------------------------------------------------
    void CopyOp::resolve_true(void)
    //--------------------------------------------------------------------------
    {
      // Put this on the queue of stuff to do
      runtime->add_to_local_queue(parent_ctx->get_executing_processor(),
                                  this, false/*prev fail*/);
    }

    //--------------------------------------------------------------------------
    void CopyOp::resolve_false(void)
    //--------------------------------------------------------------------------
    {
      // Mark that this operation has completed both
      // execution and mapping indicating that we are done
      // Do it in this order to avoid calling 'execute_trigger'
      complete_execution();
      complete_mapping();
    }

    //--------------------------------------------------------------------------
    bool CopyOp::speculate(bool &value)
    //--------------------------------------------------------------------------
    {
      Processor exec_proc = parent_ctx->get_executing_processor();
      return runtime->invoke_mapper_speculate(exec_proc, this, value);
    }

    //--------------------------------------------------------------------------
    bool CopyOp::trigger_execution(void)
    //--------------------------------------------------------------------------
    {
#ifdef LEGION_LOGGING
      LegionLogging::log_timing_event(Machine::get_executing_processor(),
                                      unique_op_id, BEGIN_MAPPING);
#endif
#ifdef LEGION_PROF
      LegionProf::register_event(unique_op_id, PROF_BEGIN_MAP_ANALYSIS);
#endif
      bool map_success = true;
      std::vector<RegionTreeContext> src_contexts(src_requirements.size());
      std::vector<RegionTreeContext> dst_contexts(dst_requirements.size());
      // Premap all the regions if we haven't already done so
      bool premapped = true;
      Processor local_proc = parent_ctx->get_executing_processor();
      for (unsigned idx = 0; idx < src_requirements.size(); idx++)
      {
        src_contexts[idx] = parent_ctx->find_enclosing_physical_context(
                                              src_requirements[idx].parent);
        if (!src_requirements[idx].premapped)
        {
          src_requirements[idx].premapped = 
            runtime->forest->premap_physical_region(
                  src_contexts[idx],src_privilege_paths[idx],
                  src_requirements[idx], 
                  this, parent_ctx, local_proc
#ifdef DEBUG_HIGH_LEVEL
                  , idx, get_logging_name(), unique_op_id
#endif
                  );
        }
        if (!src_requirements[idx].premapped)
          premapped = false;
      }
      for (unsigned idx = 0; idx < dst_requirements.size(); idx++)
      {
        unsigned index = src_requirements.size() + idx;
        dst_contexts[idx] = parent_ctx->find_enclosing_physical_context(
                                              dst_requirements[idx].parent);
        if (!dst_requirements[idx].premapped)
        {
          dst_requirements[idx].premapped = 
            runtime->forest->premap_physical_region(
                  dst_contexts[idx],dst_privilege_paths[idx],
                  dst_requirements[idx], 
                  this, parent_ctx, local_proc
#ifdef DEBUG_HIGH_LEVEL
                  , src_requirements.size()+idx
                  , get_logging_name(), unique_op_id
#endif
                  );
        }
        if (!dst_requirements[idx].premapped)
          premapped = false;
      }
      // If we couldn't premap, then we need to try again later
      if (!premapped)
        return false;
      // Now ask the mapper how to map this copy operation
      bool notify = runtime->invoke_mapper_map_copy(local_proc, this);
      // Map all the destination instances
      std::vector<MappingRef> src_mapping_refs(src_requirements.size());
      for (unsigned idx = 0; (idx < src_requirements.size()) && 
            map_success; idx++)
      {
        // If there is no ranking for this instance then we can skip
        // it as we will just issue copies from the existing valid instances
        if (src_requirements[idx].target_ranking.empty())
          continue;
        src_mapping_refs[idx] = runtime->forest->map_physical_region(
                                                        src_contexts[idx],
                                                        src_mapping_paths[idx],
                                                        src_requirements[idx],
                                                        idx,
                                                        this,
                                                        local_proc,
                                                        local_proc
#ifdef DEBUG_HIGH_LEVEL
                                                        , get_logging_name()
                                                        , unique_op_id
#endif
                                                        );
        if (!src_mapping_refs[idx].has_ref())
        {
          map_success = false;
          src_requirements[idx].mapping_failed = true;
          src_requirements[idx].selected_memory = Memory::NO_MEMORY;
          break;
        }
      }
      std::vector<MappingRef> dst_mapping_refs(dst_requirements.size());
      for (unsigned idx = 0; (idx < dst_requirements.size()) && 
            map_success; idx++)
      {
        dst_mapping_refs[idx] = runtime->forest->map_physical_region(
                                                    dst_contexts[idx],
                                                    dst_mapping_paths[idx],
                                                    dst_requirements[idx],
                                                    src_requirements.size()+idx,
                                                    this,
                                                    local_proc,
                                                    local_proc
#ifdef DEBUG_HIGH_LEVEL
                                                    , get_logging_name()
                                                    , unique_op_id
#endif
                                                    );
        if (!dst_mapping_refs[idx].has_ref())
        {
          map_success = false;
          dst_requirements[idx].mapping_failed = true;
          dst_requirements[idx].selected_memory = Memory::NO_MEMORY;
          // Break out since we failed
          break;
        }
      }

      // If we successfully mapped, then we can issue the copies
      // These should be guaranteed to succeed since no new 
      // regions will need to be created because we already
      // made all the destination regions.
      if (map_success)
      {
        // First get the set of barriers and grants we need to wait 
        // on before we can start running this copy.
        Event sync_precondition = Event::NO_EVENT;
        if (!wait_barriers.empty() || !grants.empty())
        {
          std::set<Event> preconditions;
          for (std::vector<PhaseBarrier>::const_iterator it = 
                wait_barriers.begin(); it != wait_barriers.end(); it++)
          {
            Event e = it->phase_barrier.get_previous_phase(); 
            preconditions.insert(e);
          }
          for (std::vector<Grant>::const_iterator it = grants.begin();
                it != grants.end(); it++)
          {
            Event e = it->impl->acquire_grant();
            preconditions.insert(e);
          }
          sync_precondition = Event::merge_events(preconditions);
#if defined(LEGION_LOGGING) || defined(LEGION_SPY)
          if (!sync_precondition.exists())
          {
            UserEvent new_pre = UserEvent::create_user_event();
            new_pre.trigger();
            sync_precondition = new_pre;
          }
#endif
#ifdef LEGION_LOGGING
          LegionLogging::log_event_dependences(
              Machine::get_executing_processor(), preconditions,
                                              sync_precondition);
#endif
#ifdef LEGION_SPY
          LegionSpy::log_event_dependences(preconditions,
                                           sync_precondition);
#endif
        }
#ifdef LEGION_SPY
        std::set<Event> start_events;
        start_events.insert(sync_precondition);
#endif
        std::set<Event> copy_complete_events;
        for (unsigned idx = 0; idx < src_requirements.size(); idx++)
        {
          InstanceRef dst_ref = runtime->forest->register_physical_region(
                                                        dst_contexts[idx],
                                                        dst_mapping_refs[idx],
                                                        dst_requirements[idx],
                                                        idx,
                                                        this,
                                                        local_proc,
                                                        completion_event
#ifdef DEBUG_HIGH_LEVEL
                                                        , get_logging_name()
                                                        , unique_op_id
                                                        , dst_mapping_paths[idx]
#endif
                                                        );
#ifdef DEBUG_HIGH_LEVEL
          assert(dst_ref.has_ref());
#endif
          if (notify)
          {
            dst_requirements[idx].mapping_failed = false;
            dst_requirements[idx].selected_memory = dst_ref.get_memory();
          }
#ifdef LEGION_SPY
          start_events.insert(dst_ref.get_ready_event());
          LegionSpy::log_op_user(unique_op_id, src_requirements.size()+idx,
             dst_ref.get_handle().get_view()->get_manager()->get_instance().id);
#endif
          if (!src_mapping_refs[idx].has_ref())
          {
            // In this case, there is no source instance so we need
            // to issue copies from the valid set
            copy_complete_events.insert(runtime->forest->copy_across(this, 
                                          parent_ctx->get_executing_processor(),
                                                   src_contexts[idx],
                                                   dst_contexts[idx],
                                                   src_requirements[idx],
                                                   dst_requirements[idx],
                                                   dst_ref, sync_precondition));
          }
          else
          {
            InstanceRef src_ref = runtime->forest->register_physical_region(
                                                        src_contexts[idx],
                                                        src_mapping_refs[idx],
                                                        src_requirements[idx],
                                                        idx,
                                                        this,
                                                        local_proc,
                                                        completion_event
#ifdef DEBUG_HIGH_LEVEL
                                                        , get_logging_name()
                                                        , unique_op_id
                                                        , src_mapping_paths[idx]
#endif
                                                        );
            
#ifdef DEBUG_HIGH_LEVEL
            assert(src_ref.has_ref());
#endif
            if (notify)
            {
              src_requirements[idx].mapping_failed = false;
              src_requirements[idx].selected_memory = src_ref.get_memory();
            }
            // Now issue the copies from source to destination
            copy_complete_events.insert(
             runtime->forest->copy_across(src_contexts[idx],
                                          dst_contexts[idx],
                                          src_requirements[idx],
                                          dst_requirements[idx],
                                          src_ref, dst_ref, sync_precondition));
#ifdef LEGION_SPY
            start_events.insert(src_ref.get_ready_event());
            LegionSpy::log_op_user(unique_op_id, idx,
             src_ref.get_handle().get_view()->get_manager()->get_instance().id);
#endif
          }
        }
#ifdef LEGION_LOGGING
        LegionLogging::log_timing_event(Machine::get_executing_processor(),
                                        unique_op_id, END_MAPPING);
#endif
#ifdef LEGION_PROF
        LegionProf::register_event(unique_op_id, PROF_END_MAP_ANALYSIS);
#endif
        // Launch the complete task if necessary 
        Event copy_complete_event = 
          Event::merge_events(copy_complete_events);
#if defined(LEGION_LOGGING) || defined(LEGION_SPY)
        if (!copy_complete_event.exists())
        {
          UserEvent new_copy_complete = UserEvent::create_user_event();
          new_copy_complete.trigger();
          copy_complete_event = new_copy_complete;
        }
#endif
#ifdef LEGION_LOGGING
        LegionLogging::log_event_dependences(Machine::get_executing_processor(),
                                    copy_complete_events, copy_complete_event);
#endif
#ifdef LEGION_SPY
        Event start_event = Event::merge_events(start_events);
        if (!start_event.exists())
        {
          UserEvent new_start_event = UserEvent::create_user_event();
          new_start_event.trigger();
          start_event = new_start_event;
        }
        LegionSpy::log_event_dependences(start_events, start_event);
        LegionSpy::log_op_events(unique_op_id, start_event,
                                 completion_event);
        LegionSpy::log_event_dependences(copy_complete_events, 
                                         copy_complete_event);
        LegionSpy::log_event_dependence(copy_complete_event,
                                        completion_event);
#endif
        // Chain all the unlock and barrier arrivals off of the
        // copy complete event
        if (!arrive_barriers.empty())
        {
          for (std::vector<PhaseBarrier>::const_iterator it = 
                arrive_barriers.begin(); it != arrive_barriers.end(); it++)
          {
            it->phase_barrier.arrive(1/*count*/, copy_complete_event);    
#ifdef LEGION_LOGGING
            LegionLogging::log_event_dependence(
                Machine::get_executing_processor(),       
                copy_complete_event, it->phase_barrier);
#endif
#ifdef LEGION_SPY
            LegionSpy::log_event_dependence(copy_complete_event, 
                                            it->phase_barrier);
#endif
          }
        }

        // Mark that we completed mapping
        complete_mapping();
        // Notify the mapper if it wanted to be notified
        if (notify)
          runtime->invoke_mapper_notify_result(local_proc, this);

#ifdef LEGION_LOGGING
        LegionLogging::log_event_dependence(Machine::get_executing_processor(),
                                            copy_complete_event,
                                            completion_event);
#endif
        // Handle the case for marking when the copy completes
        if (!copy_complete_event.has_triggered())
        {
          // Issue a deferred trigger on our completion event
          // and mark that we are no longer responsible for
          // triggering our completion event.
          completion_event.trigger(copy_complete_event);
          need_completion_trigger = false;
#ifdef SPECIALIZED_UTIL_PROCS
          Processor util = runtime->get_cleanup_proc(local_proc);
#else
          Processor util = runtime->find_utility_group();
#endif
          DeferredCompleteArgs deferred_complete_args;
          deferred_complete_args.hlr_id = HLR_DEFERRED_COMPLETE_ID;
          deferred_complete_args.proxy_this = this;
          util.spawn(HLR_TASK_ID, &deferred_complete_args, 
                      sizeof(deferred_complete_args), copy_complete_event);
        }
        else
          deferred_complete();
      }
      else
      {
#ifdef LEGION_PROF
        LegionProf::register_event(unique_op_id, PROF_END_MAP_ANALYSIS);
#endif
        // We failed to map, so notify the mapper
        runtime->invoke_mapper_failed_mapping(local_proc, this);
        // Clear our our instances that did map so we start
        // again next time.
        src_mapping_refs.clear();
        dst_mapping_refs.clear();
      }
      return map_success;
    }

    //--------------------------------------------------------------------------
    void CopyOp::deferred_complete(void)
    //--------------------------------------------------------------------------
    {
      // Mark that we're done executing
      complete_execution();
    }

    //--------------------------------------------------------------------------
    void CopyOp::report_aliased_requirements(unsigned idx1, unsigned idx2)
    //--------------------------------------------------------------------------
    {
      bool is_src1 = idx1 < src_requirements.size();
      bool is_src2 = idx2 < src_requirements.size();
      unsigned actual_idx1 = is_src1 ? idx1 : (idx1 - src_requirements.size());
      unsigned actual_idx2 = is_src2 ? idx2 : (idx2 - src_requirements.size());
      log_run(LEVEL_ERROR,"Aliased region requirements for copy operations "
                          "are not permitted. Region requirement %d of %s "
                          "requirements and %d of %s requirements aliased for "
                          "copy operation (UID %lld) in task %s (UID %lld).",
                          actual_idx1, is_src1 ? "source" : "destination",
                          actual_idx2, is_src2 ? "source" : "destination",
                          unique_op_id, parent_ctx->variants->name,
                          parent_ctx->get_unique_task_id());
#ifdef DEBUG_HIGH_LEVEL
      assert(false);
#endif
      exit(ERROR_ALIASED_REGION_REQUIREMENTS);
    }

    //--------------------------------------------------------------------------
    Mappable::MappableKind CopyOp::get_mappable_kind(void) const
    //--------------------------------------------------------------------------
    {
      return COPY_MAPPABLE;
    }

    //--------------------------------------------------------------------------
    Task* CopyOp::as_mappable_task(void) const
    //--------------------------------------------------------------------------
    {
      return NULL;
    }

    //--------------------------------------------------------------------------
    Copy* CopyOp::as_mappable_copy(void) const
    //--------------------------------------------------------------------------
    {
      CopyOp *proxy_this = const_cast<CopyOp*>(this);
      return proxy_this;
    }
    
    //--------------------------------------------------------------------------
    Inline* CopyOp::as_mappable_inline(void) const
    //--------------------------------------------------------------------------
    {
      return NULL;
    }
    
    //--------------------------------------------------------------------------
    Acquire* CopyOp::as_mappable_acquire(void) const
    //--------------------------------------------------------------------------
    {
      return NULL;
    }

    //--------------------------------------------------------------------------
    Release* CopyOp::as_mappable_release(void) const
    //--------------------------------------------------------------------------
    {
      return NULL;
    }

    //--------------------------------------------------------------------------
    UniqueID CopyOp::get_unique_mappable_id(void) const
    //--------------------------------------------------------------------------
    {
      return unique_op_id; 
    }

    //--------------------------------------------------------------------------
    void CopyOp::check_copy_privilege(const RegionRequirement &requirement, 
                                      unsigned idx, bool src)
    //--------------------------------------------------------------------------
    {
      if ((requirement.handle_type == PART_PROJECTION) ||
          (requirement.handle_type == REG_PROJECTION))
      {
        log_region(LEVEL_ERROR,"Projection region requirements are not "
                               "permitted for copy operations (in task %s)",
                               parent_ctx->variants->name);
#ifdef DEBUG_HIGH_LEVEL
        assert(false);
#endif
        exit(ERROR_BAD_PROJECTION_USE);
      }
      FieldID bad_field;
      LegionErrorType et = runtime->verify_requirement(requirement, bad_field);
      // If that worked, then check the privileges with the parent context
      if (et == NO_ERROR)
        et = parent_ctx->check_privilege(requirement, bad_field);
      switch (et)
      {
        case NO_ERROR:
          break;
        case ERROR_INVALID_REGION_HANDLE:
          {
            log_region(LEVEL_ERROR,"Requirest for invalid region handle "
                                   "(" IDFMT ",%d,%d) for index %d of %s "
                                   "requirements of copy operation (ID %lld)",
                                   requirement.region.index_space.id, 
                                   requirement.region.field_space.id, 
                                   requirement.region.tree_id, 
                                   idx, (src ? "source" : "destination"),
                                   unique_op_id);
#ifdef DEBUG_HIGH_LEVEL
            assert(false);
#endif
            exit(ERROR_INVALID_REGION_HANDLE);
          }
        case ERROR_FIELD_SPACE_FIELD_MISMATCH:
          {
            FieldSpace sp = (requirement.handle_type == SINGULAR) || 
                            (requirement.handle_type == REG_PROJECTION)
                             ? requirement.region.field_space : 
                               requirement.partition.field_space;
            log_region(LEVEL_ERROR,"Field %d is not a valid field of field "
                                   "space %d for index %d of %s requirements"
                                   "of copy operation (ID %lld)",
                                   bad_field, sp.id, idx, 
                                   (src ? "source" : "destination"),
                                   unique_op_id);
#ifdef DEBUG_HIGH_LEVEL
            assert(false);
#endif
            exit(ERROR_FIELD_SPACE_FIELD_MISMATCH);
          }
        case ERROR_INVALID_INSTANCE_FIELD:
          {
            log_region(LEVEL_ERROR,"Instance field %d is not one of the "
                                   "privilege fields for index %d of %s "
                                   "requirements of copy operation (ID %lld)",
                                    bad_field, idx, 
                                    (src ? "source" : "destination"),
                                    unique_op_id);
#ifdef DEBUG_HIGH_LEVEL
            assert(false);
#endif
            exit(ERROR_INVALID_INSTANCE_FIELD);
          }
        case ERROR_DUPLICATE_INSTANCE_FIELD:
          {
            log_region(LEVEL_ERROR, "Instance field %d is a duplicate for "
                                    "index %d of %s requirements of copy "
                                    "operation (ID %lld)",
                                    bad_field, idx,
                                    (src ? "source" : "destination"),
                                    unique_op_id);
#ifdef DEBUG_HIGH_LEVEL
            assert(false);
#endif
            exit(ERROR_DUPLICATE_INSTANCE_FIELD);
          }
        case ERROR_BAD_PARENT_REGION:
          {
            log_region(LEVEL_ERROR,"Parent task %s (ID %lld) of copy operation "
                                   "(ID %lld) does not have a region "
                                   "requirement for region (" IDFMT ",%x,%x) "
                                   "as a parent of index %d of %s region "
                                   "requirements",
                                   parent_ctx->variants->name, 
                                   parent_ctx->get_unique_task_id(),
                                   unique_op_id, 
                                   requirement.region.index_space.id,
                                   requirement.region.field_space.id, 
                                   requirement.region.tree_id,
                                   idx, (src ? "source" : "destination"));
#ifdef DEBUG_HIGH_LEVEL
            assert(false);
#endif
            exit(ERROR_BAD_PARENT_REGION);
          }
        case ERROR_BAD_REGION_PATH:
          {
            log_region(LEVEL_ERROR,"Region (" IDFMT ",%x,%x) is not a "
                                   "sub-region of parent region (" IDFMT 
                                   ",%x,%x) for index %d of "
                                   "%s region requirements of copy "
                                   "operation (ID %lld)",
                                   requirement.region.index_space.id,
                                   requirement.region.field_space.id, 
                                   requirement.region.tree_id,
                                   requirement.parent.index_space.id,
                                   requirement.parent.field_space.id,
                                   requirement.parent.tree_id,
                                   idx, (src ? "source" : "destination"),
                                   unique_op_id);
#ifdef DEBUG_HIGH_LEVEL
            assert(false);
#endif
            exit(ERROR_BAD_REGION_PATH);
          }
        case ERROR_BAD_REGION_TYPE:
          {
            log_region(LEVEL_ERROR,"Region requirement of copy operation "
                                   "(ID %lld) cannot find privileges for field "
                                   "%d in parent task from index %d of %s "
                                   "region requirements",
                                   unique_op_id, bad_field, idx,
                                   (src ? "source" : "destination"));
#ifdef DEBUG_HIGH_LEVEL
            assert(false);
#endif
            exit(ERROR_BAD_REGION_TYPE);
          }
        case ERROR_BAD_REGION_PRIVILEGES:
          {
            log_region(LEVEL_ERROR,"Privileges %x for region (" IDFMT 
                                   ",%x,%x) are "
                                   "not a subset of privileges of parent "
                                   "task's privileges for index %d of %s "
                                   "region requirements for copy "
                                   "operation (ID %lld)",
                                   requirement.privilege, 
                                   requirement.region.index_space.id,
                                   requirement.region.field_space.id, 
                                   requirement.region.tree_id, 
                                   idx, (src ? "source" : "destination"),
                                   unique_op_id);
#ifdef DEBUG_HIGH_LEVEL
            assert(false);
#endif
            exit(ERROR_BAD_REGION_PRIVILEGES);
          }
        // this should never happen with an inline mapping
        case ERROR_NON_DISJOINT_PARTITION: 
        default:
          assert(false); // Should never happen
      }
    }

    /////////////////////////////////////////////////////////////
    // Fence Operation 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    FenceOp::FenceOp(Runtime *rt)
      : Operation(rt)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    FenceOp::FenceOp(const FenceOp &rhs)
      : Operation(NULL)
    //--------------------------------------------------------------------------
    {
      // should never be called
      assert(false);
    }

    //--------------------------------------------------------------------------
    FenceOp::~FenceOp(void)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    FenceOp& FenceOp::operator=(const FenceOp &rhs)
    //--------------------------------------------------------------------------
    {
      // should never be called
      assert(false);
      return *this;
    }

    //--------------------------------------------------------------------------
    void FenceOp::initialize(SingleTask *ctx, bool mapping)
    //--------------------------------------------------------------------------
    {
      initialize_operation(ctx, true/*track*/, Event::NO_EVENT);
      mapping_fence = mapping;
#ifdef LEGION_LOGGING
      LegionLogging::log_fence_operation(parent_ctx->get_executing_processor(),
                                         parent_ctx->get_unique_op_id(),
                                         unique_op_id);
#endif
#ifdef LEGION_SPY
      LegionSpy::log_fence_operation(parent_ctx->get_unique_task_id(),
                                     unique_op_id);
#endif
    }

    //--------------------------------------------------------------------------
    void FenceOp::activate(void)
    //--------------------------------------------------------------------------
    {
      activate_operation();
    }

    //--------------------------------------------------------------------------
    void FenceOp::deactivate(void)
    //--------------------------------------------------------------------------
    {
      deactivate_operation();
      runtime->free_fence_op(this);
    }

    //--------------------------------------------------------------------------
    const char* FenceOp::get_logging_name(void)
    //--------------------------------------------------------------------------
    {
      return "Fence";
    }

    //--------------------------------------------------------------------------
    void FenceOp::trigger_dependence_analysis(void)
    //--------------------------------------------------------------------------
    {
#ifdef LEGION_LOGGING
      LegionLogging::log_timing_event(Machine::get_executing_processor(),
                                      unique_op_id, BEGIN_DEPENDENCE_ANALYSIS);
#endif
      begin_dependence_analysis();
      // Register this fence with all previous users in the parent's context
      RegionTreeContext ctx = parent_ctx->get_context();
      for (unsigned idx = 0; idx < parent_ctx->regions.size(); idx++)
      {
#ifdef DEBUG_HIGH_LEVEL
        assert(parent_ctx->regions[idx].handle_type == SINGULAR);
#endif
        runtime->forest->perform_fence_analysis(ctx, this, 
                                  parent_ctx->regions[idx].region);
      }
      // Now update the parent context with this fence
      // before we can complete the dependence analysis
      // and possibly be deactivated
      parent_ctx->update_current_fence(this);
      end_dependence_analysis();
#ifdef LEGION_LOGGING
      LegionLogging::log_timing_event(Machine::get_executing_processor(),
                                      unique_op_id, END_DEPENDENCE_ANALYSIS);
#endif
    }

    //--------------------------------------------------------------------------
    bool FenceOp::trigger_execution(void)
    //--------------------------------------------------------------------------
    {
      if (mapping_fence)
      {
        // Mark that we are done mapping and done executing
        complete_mapping();
        complete_execution();
      }
      else
      {
        // Go through and launch a completion task dependent upon
        // all the completion events of our incoming dependences.
        // Make sure that the events that we pulled out our still valid.
        // Not since we are performing this operation, then we know
        // that we are mapped and therefore our set of input dependences
        // have been fixed so we can read them without holding the lock.
        std::set<Event> trigger_events;
        for (std::map<Operation*,GenerationID>::const_iterator it = 
              incoming.begin(); it != incoming.end(); it++)
        {
          Event complete = it->first->get_completion_event();
          if (it->second == it->first->get_generation())
            trigger_events.insert(complete);
        }
        Event wait_on = Event::merge_events(trigger_events);
        if (!wait_on.has_triggered())
        {
#ifdef SPECIALIZED_UTIL_PROCS
          Processor util = runtime->get_cleanup_proc(
                            parent_ctx->get_executing_processor());
#else
          Processor util = runtime->find_utility_group();
#endif
          DeferredCompleteArgs deferred_complete_args;
          deferred_complete_args.hlr_id = HLR_DEFERRED_COMPLETE_ID;
          deferred_complete_args.proxy_this = this;
          util.spawn(HLR_TASK_ID, &deferred_complete_args,
                     sizeof(deferred_complete_args), wait_on);
        }
        else
          deferred_complete();
      }
      // If we successfully performed the operation return true
      return true;
    }

    //--------------------------------------------------------------------------
    void FenceOp::deferred_complete(void)
    //--------------------------------------------------------------------------
    {
      // Mark that we are done mapping and executing
      complete_mapping();
      complete_execution();
    } 

    /////////////////////////////////////////////////////////////
    // Deletion Operation 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    DeletionOp::DeletionOp(Runtime *rt)
      : Operation(rt)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    DeletionOp::DeletionOp(const DeletionOp &rhs)
      : Operation(NULL)
    //--------------------------------------------------------------------------
    {
      // should never be called
      assert(false);
    }

    //--------------------------------------------------------------------------
    DeletionOp::~DeletionOp(void)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    DeletionOp& DeletionOp::operator=(const DeletionOp &rhs)
    //--------------------------------------------------------------------------
    {
      // should never be called
      assert(false);
      return *this;
    }

    //--------------------------------------------------------------------------
    void DeletionOp::initialize_index_space_deletion(SingleTask *ctx,
                                                     IndexSpace handle)
    //--------------------------------------------------------------------------
    {
      initialize_operation(ctx, true/*track*/, Event::NO_EVENT);
      kind = INDEX_SPACE_DELETION;
      index_space = handle;
#ifdef LEGION_LOGGING
      LegionLogging::log_deletion_operation(
          parent_ctx->get_executing_processor(),
          parent_ctx->get_unique_op_id(), unique_op_id);
#endif
#ifdef LEGION_SPY
      LegionSpy::log_deletion_operation(parent_ctx->get_unique_task_id(),
                                        unique_op_id);
#endif
    }

    //--------------------------------------------------------------------------
    void DeletionOp::initialize_index_part_deletion(SingleTask *ctx,
                                                    IndexPartition handle)
    //--------------------------------------------------------------------------
    {
      initialize_operation(ctx, true/*track*/, Event::NO_EVENT);
      kind = INDEX_PARTITION_DELETION;
      index_part = handle;
#ifdef LEGION_LOGGING
      LegionLogging::log_deletion_operation(
          parent_ctx->get_executing_processor(),
          parent_ctx->get_unique_op_id(), unique_op_id);
#endif
#ifdef LEGION_SPY
      LegionSpy::log_deletion_operation(parent_ctx->get_unique_task_id(),
                                        unique_op_id);
#endif
    }

    //--------------------------------------------------------------------------
    void DeletionOp::initialize_field_space_deletion(SingleTask *ctx,
                                                     FieldSpace handle)
    //--------------------------------------------------------------------------
    {
      initialize_operation(ctx, true/*track*/, Event::NO_EVENT);
      kind = FIELD_SPACE_DELETION;
      field_space = handle;
#ifdef LEGION_LOGGING
      LegionLogging::log_deletion_operation(
          parent_ctx->get_executing_processor(),
          parent_ctx->get_unique_op_id(), unique_op_id);
#endif
#ifdef LEGION_SPY
      LegionSpy::log_deletion_operation(parent_ctx->get_unique_task_id(),
                                        unique_op_id);
#endif
    }

    //--------------------------------------------------------------------------
    void DeletionOp::initialize_field_deletion(SingleTask *ctx, 
                                                FieldSpace handle, FieldID fid)
    //--------------------------------------------------------------------------
    {
      initialize_operation(ctx, true/*track*/, Event::NO_EVENT);
      kind = FIELD_DELETION;
      field_space = handle;
      free_fields.insert(fid);
#ifdef LEGION_LOGGING
      LegionLogging::log_deletion_operation(
          parent_ctx->get_executing_processor(),
          parent_ctx->get_unique_op_id(), unique_op_id);
#endif
#ifdef LEGION_SPY
      LegionSpy::log_deletion_operation(parent_ctx->get_unique_task_id(),
                                        unique_op_id);
#endif
    }

    //--------------------------------------------------------------------------
    void DeletionOp::initialize_field_deletions(SingleTask *ctx,
                            FieldSpace handle, const std::set<FieldID> &to_free)
    //--------------------------------------------------------------------------
    {
      initialize_operation(ctx, true/*track*/, Event::NO_EVENT);
      kind = FIELD_DELETION;
      field_space = handle;
      free_fields = to_free;
#ifdef LEGION_LOGGING
      LegionLogging::log_deletion_operation(
          parent_ctx->get_executing_processor(),
          parent_ctx->get_unique_op_id(), unique_op_id);
#endif
#ifdef LEGION_SPY
      LegionSpy::log_deletion_operation(parent_ctx->get_unique_task_id(),
                                        unique_op_id);
#endif
    }

    //--------------------------------------------------------------------------
    void DeletionOp::initialize_logical_region_deletion(SingleTask *ctx,
                                                        LogicalRegion handle)
    //--------------------------------------------------------------------------
    {
      initialize_operation(ctx, true/*track*/, Event::NO_EVENT);
      kind = LOGICAL_REGION_DELETION;
      logical_region = handle;
#ifdef LEGION_LOGGING
      LegionLogging::log_deletion_operation(
          parent_ctx->get_executing_processor(),
          parent_ctx->get_unique_op_id(), unique_op_id);
#endif
#ifdef LEGION_SPY
      LegionSpy::log_deletion_operation(parent_ctx->get_unique_task_id(),
                                        unique_op_id);
#endif
    }

    //--------------------------------------------------------------------------
    void DeletionOp::initialize_logical_partition_deletion(SingleTask *ctx,
                                                       LogicalPartition handle)
    //--------------------------------------------------------------------------
    {
      initialize_operation(ctx, true/*track*/, Event::NO_EVENT);
      kind = LOGICAL_PARTITION_DELETION;
      logical_part = handle;
#ifdef LEGION_LOGGING
      LegionLogging::log_deletion_operation(
          parent_ctx->get_executing_processor(),
          parent_ctx->get_unique_op_id(), unique_op_id);
#endif
#ifdef LEGION_SPY
      LegionSpy::log_deletion_operation(parent_ctx->get_unique_task_id(),
                                        unique_op_id);
#endif
    }

    //--------------------------------------------------------------------------
    void DeletionOp::activate(void)
    //--------------------------------------------------------------------------
    {
      activate_operation();
    }

    //--------------------------------------------------------------------------
    void DeletionOp::deactivate(void)
    //--------------------------------------------------------------------------
    {
      deactivate_operation();
      free_fields.clear();
      // Return this to the available deletion ops on the queue
      runtime->free_deletion_op(this);
    }

    //--------------------------------------------------------------------------
    const char* DeletionOp::get_logging_name(void)
    //--------------------------------------------------------------------------
    {
      return "Deletion";
    }

    //--------------------------------------------------------------------------
    void DeletionOp::trigger_dependence_analysis(void)
    //--------------------------------------------------------------------------
    {
#ifdef LEGION_LOGGING
      LegionLogging::log_timing_event(Machine::get_executing_processor(),
                                      unique_op_id, BEGIN_DEPENDENCE_ANALYSIS);
#endif
      begin_dependence_analysis();
      switch (kind)
      {
        case INDEX_SPACE_DELETION:
          {
            parent_ctx->analyze_destroy_index_space(index_space, this);
            break;
          }
        case INDEX_PARTITION_DELETION:
          {
            parent_ctx->analyze_destroy_index_partition(index_part, this);
            break;
          }
        case FIELD_SPACE_DELETION:
          {
            parent_ctx->analyze_destroy_field_space(field_space, this);
            break;
          }
        case FIELD_DELETION:
          {
            parent_ctx->analyze_destroy_fields(field_space, this, free_fields);
            break;
          }
        case LOGICAL_REGION_DELETION:
          {
            parent_ctx->analyze_destroy_logical_region(logical_region, this);
            break;
          }
        case LOGICAL_PARTITION_DELETION:
          {
            parent_ctx->analyze_destroy_logical_partition(logical_part, this);
            break;
          }
        default:
          // should never get here
          assert(false);
      }
      end_dependence_analysis();
#ifdef LEGION_LOGGING
      LegionLogging::log_timing_event(Machine::get_executing_processor(),
                                      unique_op_id, END_DEPENDENCE_ANALYSIS);
#endif
    }

    //--------------------------------------------------------------------------
    void DeletionOp::trigger_commit(void)
    //--------------------------------------------------------------------------
    {
      // By not actually doing any operations until commit time we ensure
      // that the deletions don't actually impact the region tree state
      // until we know that it is safe to do so.
      switch (kind)
      {
        case INDEX_SPACE_DELETION:
          {
            bool top_level = runtime->finalize_index_space_destroy(index_space);
            if (top_level)
              parent_ctx->register_index_space_deletion(index_space);
            break;
          }
        case INDEX_PARTITION_DELETION:
          {
            runtime->finalize_index_partition_destroy(index_part);
            break;
          }
        case FIELD_SPACE_DELETION:
          {
            runtime->finalize_field_space_destroy(field_space);
            parent_ctx->register_field_space_deletion(field_space);
            break;
          }
        case FIELD_DELETION:
          {
            runtime->finalize_field_destroy(field_space, free_fields);
            parent_ctx->register_field_deletions(field_space, free_fields);
            break;
          }
        case LOGICAL_REGION_DELETION:
          {
            bool top_level = runtime->finalize_logical_region_destroy(
                                                        logical_region);
            // If this was a top-level region destruction
            // tell the enclosing parent task it has lost privileges
            if (top_level)
              parent_ctx->register_region_deletion(logical_region);
            break;
          }
        case LOGICAL_PARTITION_DELETION:
          {
            runtime->finalize_logical_partition_destroy(logical_part);
            break;
          }
        default:
          assert(false); // should never get here
      }
#ifdef LEGION_LOGGING
      LegionLogging::log_operation_events(
          Machine::get_executing_processor(),
          unique_op_id, Event::NO_EVENT, completion_event);
#endif
      // Commit this operation
      commit_operation();
      // Then deactivate it
      deactivate();
    }

    /////////////////////////////////////////////////////////////
    // Close Operation 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    CloseOp::CloseOp(Runtime *rt)
      : Operation(rt)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    CloseOp::CloseOp(const CloseOp &rhs)
      : Operation(NULL)
    //--------------------------------------------------------------------------
    {
      // should never be called
      assert(false);
    }

    //--------------------------------------------------------------------------
    CloseOp::~CloseOp(void)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    CloseOp& CloseOp::operator=(const CloseOp &rhs)
    //--------------------------------------------------------------------------
    {
      // should never be called
      assert(false);
      return *this;
    }

    //--------------------------------------------------------------------------
    void CloseOp::initialize(SingleTask *ctx, unsigned idx, 
                             const InstanceRef &ref)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_HIGH_LEVEL
      assert(completion_event.exists());
#endif
      initialize_operation(ctx, true/*track*/, Event::NO_EVENT);
      reference = ref;
      requirement.copy_without_mapping_info(parent_ctx->regions[idx]);
      // If it was write-discard from the task's perspective, make it
      // read-write within the task's context
      if (requirement.privilege == WRITE_DISCARD)
        requirement.privilege = READ_WRITE;
      localize_region_requirement(requirement);
      requirement.initialize_mapping_fields();
      if (parent_ctx->has_simultaneous_coherence())
        parent_ctx->check_simultaneous_restricted(requirement);
#ifdef DEBUG_HIGH_LEVEL
      assert(reference.has_ref());
      parent_index = idx;
#endif
      initialize_privilege_path(privilege_path, requirement);
#ifdef LEGION_LOGGING
      LegionLogging::log_close_operation(parent_ctx->get_executing_processor(),
                                         parent_ctx->get_unique_op_id(),
                                         unique_op_id);
      LegionLogging::log_logical_requirement(
                                         parent_ctx->get_executing_processor(),
                                         unique_op_id, 0/*idx*/, true/*region*/,
                                         requirement.region.index_space.id,
                                         requirement.region.field_space.id,
                                         requirement.region.tree_id,
                                         requirement.privilege,
                                         requirement.prop,
                                         requirement.redop,
                                         requirement.privilege_fields);
#endif
#ifdef LEGION_PROF
      LegionProf::register_close(unique_op_id, 
                                 parent_ctx->get_unique_task_id());
#endif
#ifdef LEGION_SPY
      LegionSpy::log_close_operation(parent_ctx->get_unique_task_id(),
                                     unique_op_id);
      LegionSpy::log_logical_requirement(unique_op_id, 0/*idx*/, true/*region*/,
                                requirement.region.index_space.id,
                                requirement.region.field_space.id,
                                requirement.region.tree_id,
                                requirement.privilege,
                                requirement.prop,
                                requirement.redop);
      LegionSpy::log_requirement_fields(unique_op_id, 0/*idx*/,
                                requirement.privilege_fields);
#endif
    }

    //--------------------------------------------------------------------------
    void CloseOp::deferred_complete(void)
    //--------------------------------------------------------------------------
    {
#if defined(LEGION_PROF) || defined(LEGION_LOGGING)
      UniqueID local_id = unique_op_id;
#endif
#ifdef LEGION_PROF
      LegionProf::register_event(local_id, PROF_BEGIN_POST);
#endif
#ifdef LEGION_LOGGING
      LegionLogging::log_timing_event(Machine::get_executing_processor(),
                                      local_id,
                                      BEGIN_POST_EXEC);
#endif
      complete_execution();
#ifdef LEGION_PROF
      LegionProf::register_event(local_id, PROF_END_POST);
#endif
#ifdef LEGION_LOGGING
      LegionLogging::log_timing_event(Machine::get_executing_processor(),
                                      local_id,
                                      BEGIN_POST_EXEC);
#endif
    }

    //--------------------------------------------------------------------------
    void CloseOp::activate(void)
    //--------------------------------------------------------------------------
    {
      activate_operation();
#ifdef DEBUG_HIGH_LEVEL
      assert(completion_event.exists());
#endif
    }

    //--------------------------------------------------------------------------
    void CloseOp::deactivate(void)
    //--------------------------------------------------------------------------
    {
      deactivate_operation();
      reference = InstanceRef();
      privilege_path = RegionTreePath();
      runtime->free_close_op(this);
    }

    //--------------------------------------------------------------------------
    const char* CloseOp::get_logging_name(void)
    //--------------------------------------------------------------------------
    {
      return "Close";
    }

    //--------------------------------------------------------------------------
    void CloseOp::trigger_dependence_analysis(void)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_HIGH_LEVEL
      assert(completion_event.exists());
#endif
#ifdef LEGION_LOGGING
      LegionLogging::log_timing_event(Machine::get_executing_processor(),
                                      unique_op_id, BEGIN_DEPENDENCE_ANALYSIS);
#endif
#ifdef LEGION_PROF
      LegionProf::register_event(unique_op_id, PROF_BEGIN_DEP_ANALYSIS);
#endif
      begin_dependence_analysis();
      runtime->forest->perform_dependence_analysis(parent_ctx->get_context(),
                                                   this, 0/*idx*/,
                                                   requirement,
                                                   privilege_path);
      end_dependence_analysis();
#ifdef LEGION_LOGGING
      LegionLogging::log_timing_event(Machine::get_executing_processor(),
                                      unique_op_id, END_DEPENDENCE_ANALYSIS);
#endif
#ifdef LEGION_PROF
      LegionProf::register_event(unique_op_id, PROF_END_DEP_ANALYSIS);
#endif
    }

    //--------------------------------------------------------------------------
    bool CloseOp::trigger_execution(void)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_HIGH_LEVEL
      assert(completion_event.exists());
#endif
#ifdef LEGION_LOGGING
      LegionLogging::log_timing_event(Machine::get_executing_processor(),
                                      unique_op_id, BEGIN_MAPPING);
#endif
#ifdef LEGION_PROF
      LegionProf::register_event(unique_op_id, PROF_BEGIN_MAP_ANALYSIS);
#endif
      // Ask the region tree forest to close up to the specified instance
      // This operation should never fail since we know the physical instance
      // already exists
      Event close_event = runtime->forest->close_physical_context(
                                         parent_ctx->get_context(), 
                                         requirement,
                                         parent_ctx,
                                         parent_ctx,
                                         parent_ctx->get_executing_processor(),
                                         reference
#ifdef DEBUG_HIGH_LEVEL
                                         , parent_index
                                         , get_logging_name()
                                         , unique_op_id
#endif
                                         );
#ifdef LEGION_LOGGING
      LegionLogging::log_timing_event(Machine::get_executing_processor(),
                                      unique_op_id, END_MAPPING);
      LegionLogging::log_operation_events(
          Machine::get_executing_processor(),
          unique_op_id, Event::NO_EVENT, close_event);
      LegionLogging::log_physical_user(
          Machine::get_executing_processor(),
          reference.get_handle().get_view()->get_manager()->get_instance(),
          unique_op_id, 0/*idx*/);
#endif
#ifdef LEGION_PROF
      LegionProf::register_event(unique_op_id, PROF_END_MAP_ANALYSIS);
#endif
#ifdef LEGION_SPY
      // Log an implicit dependence on the parent's start event
      LegionSpy::log_implicit_dependence(parent_ctx->get_start_event(), 
                                         close_event);
      // Note this gives us a dependence to the parent's termination event
      LegionSpy::log_op_events(unique_op_id, close_event, 
                               parent_ctx->get_task_completion());
      LegionSpy::log_op_user(unique_op_id, 0/*idx*/, 
          reference.get_handle().get_view()->get_manager()->get_instance().id);

#endif
      complete_mapping();
#ifdef LEGION_LOGGING
      LegionLogging::log_event_dependence(Machine::get_executing_processor(),
                                          close_event,
                                          completion_event);
#endif
      // See if we need to defer completion of the close operation
      if (!close_event.has_triggered())
      {
        // Issue a deferred trigger of our completion event and mark
        // that we are no longer responsible for triggering it
        // when we are complete.
        completion_event.trigger(close_event);
        need_completion_trigger = false;
#ifdef SPECIALIZED_UTIL_PROCS
        Processor util = runtime->get_cleanup_proc(
                          parent_ctx->get_executing_processor());
#else
        Processor util = runtime->find_utility_group();
#endif
        DeferredCompleteArgs deferred_complete_args;
        deferred_complete_args.hlr_id = HLR_DEFERRED_COMPLETE_ID;
        deferred_complete_args.proxy_this = this;
        util.spawn(HLR_TASK_ID, &deferred_complete_args, 
                   sizeof(deferred_complete_args), close_event);
      }
      else
        deferred_complete();
      // This should always succeed
      return true;
    }

    /////////////////////////////////////////////////////////////
    // Acquire Operation 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    AcquireOp::AcquireOp(Runtime *rt)
      : Acquire(), SpeculativeOp(rt)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    AcquireOp::AcquireOp(const AcquireOp &rhs)
      : Acquire(), SpeculativeOp(NULL)
    //--------------------------------------------------------------------------
    {
      // should never be called
      assert(false);
    }

    //--------------------------------------------------------------------------
    AcquireOp::~AcquireOp(void)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    AcquireOp& AcquireOp::operator=(const AcquireOp &rhs)
    //--------------------------------------------------------------------------
    {
      // should never be called
      assert(false);
      return *this;
    }

    //--------------------------------------------------------------------------
    void AcquireOp::initialize(Context ctx, 
                               const AcquireLauncher &launcher,
                               bool check_privileges)
    //--------------------------------------------------------------------------
    {
      parent_ctx = ctx;
      parent_task = ctx;
      initialize_speculation(ctx, true/*track*/, Event::NO_EVENT,
                             1/*num region requirements*/,
                             launcher.predicate);
      // Note we give it READ WRITE EXCLUSIVE to make sure that nobody
      // can be re-ordered around this operation for mapping or
      // normal dependences.  We won't actually read or write anything.
      requirement = RegionRequirement(launcher.logical_region, READ_WRITE,
                                      EXCLUSIVE, launcher.parent_region);
      requirement.privilege_fields = launcher.fields;
      logical_region = launcher.logical_region;
      parent_region = launcher.parent_region;
      fields = launcher.fields;
      // Mark the requirement restricted
      region = launcher.physical_region;
      grants = launcher.grants;
      // Register ourselves with all the grants
      for (unsigned idx = 0; idx < grants.size(); idx++)
        grants[idx].impl->register_operation(completion_event);
      wait_barriers = launcher.wait_barriers;
      for (std::vector<PhaseBarrier>::const_iterator it = 
            launcher.arrive_barriers.begin(); it != 
            launcher.arrive_barriers.end(); it++)
      {
        // TODO: Put this back in once Sean fixes barriers
#if 0
        arrive_barriers.push_back(
            PhaseBarrier(it->phase_barrier.alter_arrival_count(1),
                         it->participants));
#else
        arrive_barriers.push_back(*it);
#endif
      }
      map_id = launcher.map_id;
      tag = launcher.tag;
      if (check_privileges)
        check_acquire_privilege();
      initialize_privilege_path(privilege_path, requirement);
#ifdef DEBUG_HIGH_LEVEL
      initialize_mapping_path(mapping_path, requirement, requirement.region);
#endif
    }

    //--------------------------------------------------------------------------
    void AcquireOp::activate(void)
    //--------------------------------------------------------------------------
    {
      activate_speculative();
    }

    //--------------------------------------------------------------------------
    void AcquireOp::deactivate(void)
    //--------------------------------------------------------------------------
    {
      deactivate_speculative();  
      // Remove our reference to the physical region
      region = PhysicalRegion();
      privilege_path = RegionTreePath();
#ifdef DEBUG_HIGH_LEVEL
      mapping_path = RegionTreePath();
#endif
      fields.clear();
      grants.clear();
      wait_barriers.clear();
      arrive_barriers.clear();
      // Return this operation to the runtime
      runtime->free_acquire_op(this);
    }

    //--------------------------------------------------------------------------
    const char* AcquireOp::get_logging_name(void)
    //--------------------------------------------------------------------------
    {
      return "Acquire";
    }

    //--------------------------------------------------------------------------
    void AcquireOp::trigger_dependence_analysis(void)
    //--------------------------------------------------------------------------
    {
      begin_dependence_analysis();
      // Register a dependence on our predicate
      register_predicate_dependence();
      // First register any mapping dependences that we have
      runtime->forest->perform_dependence_analysis(parent_ctx->get_context(),
                                                   this, 0/*idx*/, requirement,
                                                   privilege_path);
      // Now tell the forest that we have user-level coherence
      runtime->forest->acquire_user_coherence(parent_ctx->get_context(),
                                              requirement.region,
                                              requirement.privilege_fields);
      end_dependence_analysis();
    }

    //--------------------------------------------------------------------------
    void AcquireOp::resolve_true(void)
    //--------------------------------------------------------------------------
    {
      // Put this on the queue of stuff to do
      runtime->add_to_local_queue(parent_ctx->get_executing_processor(),
                                  this, false/*prev fail*/);
    }

    //--------------------------------------------------------------------------
    void AcquireOp::resolve_false(void)
    //--------------------------------------------------------------------------
    {
      // Clean up this operation
      complete_execution();
      complete_mapping();
    }

    //--------------------------------------------------------------------------
    bool AcquireOp::speculate(bool &value)
    //--------------------------------------------------------------------------
    {
      Processor exec_proc = parent_ctx->get_executing_processor();
      return runtime->invoke_mapper_speculate(exec_proc, this, value);
    }

    //--------------------------------------------------------------------------
    bool AcquireOp::trigger_execution(void)
    //--------------------------------------------------------------------------
    {
      // Mark our region requirement as being restricted now
      requirement.restricted = true;
      RegionTreeContext physical_ctx = 
        parent_ctx->find_enclosing_physical_context(requirement.parent);
      Processor local_proc = parent_ctx->get_executing_processor();
      // If we haven't already premapped the path, then do so now
      if (!requirement.premapped)
      {
        // Use our parent_ctx as the mappable since technically
        // we aren't a mappable.  Technically this shouldn't do anything
        // because we've marked ourselves as being restricted.
        requirement.premapped = runtime->forest->premap_physical_region(
                  physical_ctx, privilege_path, requirement, 
                  this, parent_ctx, local_proc
#ifdef DEBUG_HIGH_LEVEL
                  , 0/*idx*/, get_logging_name(), unique_op_id
#endif
                  );
      }
      // If we couldn't premap, then we need to try again later
      if (!requirement.premapped)
        return false;
      // We use 'remapping' as the mechanism for figuring out who
      // we need to wait on.  We already know the physical region 
      // that we want to map.
      MappingRef map_ref = runtime->forest->remap_physical_region(physical_ctx,
                                                                  requirement,
                                                                  0/*idx*/,
                                                  region.impl->get_reference()
#ifdef DEBUG_HIGH_LEVEL
                                                          , get_logging_name()
                                                          , unique_op_id
#endif
                                                                  );
#ifdef DEBUG_HIGH_LEVEL
      assert(map_ref.has_ref());
#endif
      InstanceRef result = runtime->forest->register_physical_region(
                                                          physical_ctx,
                                                          map_ref,
                                                          requirement,
                                                          0/*idx*/,
                                                          this,
                                                          local_proc,
                                                          completion_event
#ifdef DEBUG_HIGH_LEVEL
                                                          , get_logging_name()
                                                          , unique_op_id
                                                          , mapping_path
#endif
                                                          );
#ifdef DEBUG_HIGH_LEVEL
      assert(result.has_ref());
#endif
      // Get all the events that need to happen before we can consider
      // ourselves acquired: reference ready and all synchronization
      std::set<Event> acquire_preconditions;
      acquire_preconditions.insert(result.get_ready_event());
      if (!wait_barriers.empty())
      {
        for (std::vector<PhaseBarrier>::const_iterator it = 
              wait_barriers.begin(); it != wait_barriers.end(); it++)
        {
          Event e = it->phase_barrier.get_previous_phase();
          acquire_preconditions.insert(e);
        }
      }
      if (!grants.empty())
      {
        for (std::vector<Grant>::const_iterator it = grants.begin();
              it != grants.end(); it++)
        {
          Event e = it->impl->acquire_grant();
          acquire_preconditions.insert(e);
        }
      }
      Event acquire_complete = Event::merge_events(acquire_preconditions);
        
      // Chain any arrival barriers
      if (!arrive_barriers.empty())
      {
        for (std::vector<PhaseBarrier>::const_iterator it = 
              arrive_barriers.begin(); it != arrive_barriers.end(); it++)
        {
          it->phase_barrier.arrive(1/*count*/, acquire_complete);
        }
      }
      
      // Mark that we completed mapping
      complete_mapping();

      // See if we already triggered
      if (!acquire_complete.has_triggered())
      {
        completion_event.trigger(acquire_complete);
        need_completion_trigger = false;
#ifdef SPECIALIZED_UTIL_PROCS
        Processor util = runtime->get_cleanup_proc(local_proc);
#else
        Processor util = runtime->find_utility_group();
#endif
        DeferredCompleteArgs deferred_complete_args;
        deferred_complete_args.hlr_id = HLR_DEFERRED_COMPLETE_ID;
        deferred_complete_args.proxy_this = this;
        util.spawn(HLR_TASK_ID, &deferred_complete_args,
                    sizeof(deferred_complete_args), acquire_complete);
      }
      else
        deferred_complete();
      // we succeeded in mapping
      return true;
    }

    //--------------------------------------------------------------------------
    void AcquireOp::deferred_complete(void)
    //--------------------------------------------------------------------------
    {
      // Mark that we're done executing
      complete_execution();
    }

    //--------------------------------------------------------------------------
    Mappable::MappableKind AcquireOp::get_mappable_kind(void) const
    //--------------------------------------------------------------------------
    {
      return ACQUIRE_MAPPABLE;
    }

    //--------------------------------------------------------------------------
    Task* AcquireOp::as_mappable_task(void) const
    //--------------------------------------------------------------------------
    {
      return NULL;
    }

    //--------------------------------------------------------------------------
    Copy* AcquireOp::as_mappable_copy(void) const
    //--------------------------------------------------------------------------
    {
      return NULL;
    }

    //--------------------------------------------------------------------------
    Inline* AcquireOp::as_mappable_inline(void) const
    //--------------------------------------------------------------------------
    {
      return NULL;
    }

    //--------------------------------------------------------------------------
    Acquire* AcquireOp::as_mappable_acquire(void) const
    //--------------------------------------------------------------------------
    {
      AcquireOp *proxy_this = const_cast<AcquireOp*>(this);
      return proxy_this;
    }

    //--------------------------------------------------------------------------
    Release* AcquireOp::as_mappable_release(void) const
    //--------------------------------------------------------------------------
    {
      return NULL;
    }

    //--------------------------------------------------------------------------
    UniqueID AcquireOp::get_unique_mappable_id(void) const
    //--------------------------------------------------------------------------
    {
      return unique_op_id;
    }

    //--------------------------------------------------------------------------
    const RegionRequirement& AcquireOp::get_requirement(void) const
    //--------------------------------------------------------------------------
    {
      return requirement;
    }

    //--------------------------------------------------------------------------
    void AcquireOp::check_acquire_privilege(void)
    //--------------------------------------------------------------------------
    {
      // Check to make sure the physical region was mapped for
      // the same logical region.
      {
        const RegionRequirement &req = region.impl->get_requirement();
        if (req.region != requirement.region)
        {
          log_region(LEVEL_ERROR,"Mismatch between logical region (" IDFMT 
                                 ",%d,%d) and logical region (" IDFMT 
                                 ",%d,%d) used for mapping physical region "
                                 "for acquire operation (ID %lld)",
                                 requirement.region.index_space.id,
                                 requirement.region.field_space.id,
                                 requirement.region.tree_id,
                                 req.region.index_space.id,
                                 req.region.field_space.id,
                                 req.region.tree_id, unique_op_id);
#ifdef DEBUG_HIGH_LEVEL
          assert(false);
#endif
          exit(ERROR_ACQUIRE_MISMATCH);
        }
      }
      FieldID bad_field;
      LegionErrorType et = runtime->verify_requirement(requirement, bad_field);
      // If that worked, check the privileges, but only check the
      // data and not the actual privilege values since we're
      // using psuedo-read-write-exclusive
      if (et == NO_ERROR)
        et = parent_ctx->check_privilege(requirement, bad_field, true/*skip*/);
      switch (et)
      {
        case NO_ERROR:
          break;
        case ERROR_INVALID_REGION_HANDLE:
          {
            log_region(LEVEL_ERROR,"Requirest for invalid region handle "
                                   "(" IDFMT ",%d,%d) of requirement for "
                                   "acquire operation (ID %lld)",
                                   requirement.region.index_space.id, 
                                   requirement.region.field_space.id, 
                                   requirement.region.tree_id, 
                                   unique_op_id);
#ifdef DEBUG_HIGH_LEVEL
            assert(false);
#endif
            exit(ERROR_INVALID_REGION_HANDLE);
          }
        case ERROR_FIELD_SPACE_FIELD_MISMATCH:
          {
            FieldSpace sp = (requirement.handle_type == SINGULAR) || 
                            (requirement.handle_type == REG_PROJECTION)
                             ? requirement.region.field_space : 
                               requirement.partition.field_space;
            log_region(LEVEL_ERROR,"Field %d is not a valid field of field "
                                   "space %d of requirement for acquire "
                                   "operation (ID %lld)",
                                   bad_field, sp.id, unique_op_id);
#ifdef DEBUG_HIGH_LEVEL
            assert(false);
#endif
            exit(ERROR_FIELD_SPACE_FIELD_MISMATCH);
          }
        case ERROR_BAD_PARENT_REGION:
          {
            log_region(LEVEL_ERROR,"Parent task %s (ID %lld) of acquire "
                                   "operation (ID %lld) does not have a region "
                                   "requirement for region (" IDFMT 
                                   ",%x,%x) as a parent",
                                   parent_ctx->variants->name, 
                                   parent_ctx->get_unique_task_id(),
                                   unique_op_id, 
                                   requirement.region.index_space.id,
                                   requirement.region.field_space.id, 
                                   requirement.region.tree_id);
#ifdef DEBUG_HIGH_LEVEL
            assert(false);
#endif
            exit(ERROR_BAD_PARENT_REGION);
          }
        case ERROR_BAD_REGION_PATH:
          {
            log_region(LEVEL_ERROR,"Region (" IDFMT ",%x,%x) is not a "
                                   "sub-region of parent region (" IDFMT 
                                   ",%x,%x) of requirement "
                                   "for acquire operation (ID %lld)",
                                   requirement.region.index_space.id,
                                   requirement.region.field_space.id, 
                                   requirement.region.tree_id,
                                   requirement.parent.index_space.id,
                                   requirement.parent.field_space.id,
                                   requirement.parent.tree_id, unique_op_id);
#ifdef DEBUG_HIGH_LEVEL
            assert(false);
#endif
            exit(ERROR_BAD_REGION_PATH);
          }
        case ERROR_BAD_REGION_TYPE:
          {
            log_region(LEVEL_ERROR,"Region requirement of acquire operation "
                                   "(ID %lld) cannot find privileges for field "
                                   "%d in parent task",
                                   unique_op_id, bad_field);
#ifdef DEBUG_HIGH_LEVEL
            assert(false);
#endif
            exit(ERROR_BAD_REGION_TYPE);
          }
        // these should never happen with an acquire operation 
        case ERROR_INVALID_INSTANCE_FIELD:
        case ERROR_DUPLICATE_INSTANCE_FIELD:
        case ERROR_BAD_REGION_PRIVILEGES:
        case ERROR_NON_DISJOINT_PARTITION: 
        default:
          assert(false); // Should never happen
      }
    }

    /////////////////////////////////////////////////////////////
    // Release Operation 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    ReleaseOp::ReleaseOp(Runtime *rt)
      : Release(), SpeculativeOp(rt)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    ReleaseOp::ReleaseOp(const ReleaseOp &rhs)
      : Release(), SpeculativeOp(NULL)
    //--------------------------------------------------------------------------
    {
      // should never be called
      assert(false);
    }

    //--------------------------------------------------------------------------
    ReleaseOp::~ReleaseOp(void)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    ReleaseOp& ReleaseOp::operator=(const ReleaseOp &rhs)
    //--------------------------------------------------------------------------
    {
      // should never be called
      assert(false);
      return *this;
    }

    //--------------------------------------------------------------------------
    void ReleaseOp::initialize(Context ctx, 
                               const ReleaseLauncher &launcher, 
                               bool check_privileges)
    //--------------------------------------------------------------------------
    {
      parent_ctx = ctx;
      parent_task = ctx;
      initialize_speculation(ctx, true/*track*/, Event::NO_EVENT, 
                             1/*num region requirements*/,
                             launcher.predicate);
      // Note we give it READ WRITE EXCLUSIVE to make sure that nobody
      // can be re-ordered around this operation for mapping or
      // normal dependences.  We won't actually read or write anything.
      requirement = RegionRequirement(launcher.logical_region, READ_WRITE, 
                                      EXCLUSIVE, launcher.parent_region);
      requirement.privilege_fields = launcher.fields;
      logical_region = launcher.logical_region;
      parent_region = launcher.parent_region;
      fields = launcher.fields;
      region = launcher.physical_region;
      grants = launcher.grants;
      // Register ourselves with all the grants
      for (unsigned idx = 0; idx < grants.size(); idx++)
        grants[idx].impl->register_operation(completion_event);
      wait_barriers = launcher.wait_barriers;
      for (std::vector<PhaseBarrier>::const_iterator it = 
            launcher.arrive_barriers.begin(); it != 
            launcher.arrive_barriers.end(); it++)
      {
        // TODO: Put this back in once Sean fixes barriers
#if 0
        arrive_barriers.push_back(
            PhaseBarrier(it->phase_barrier.alter_arrival_count(1),
                         it->participants));
#else
        arrive_barriers.push_back(*it);
#endif
      }
      map_id = launcher.map_id;
      tag = launcher.tag;
      if (check_privileges)
        check_release_privilege();
      initialize_privilege_path(privilege_path, requirement);
#ifdef DEBUG_HIGH_LEVEL
      initialize_mapping_path(mapping_path, requirement, requirement.region);
#endif
    }

    //--------------------------------------------------------------------------
    void ReleaseOp::activate(void)
    //--------------------------------------------------------------------------
    {
      activate_speculative(); 
    }

    //--------------------------------------------------------------------------
    void ReleaseOp::deactivate(void)
    //--------------------------------------------------------------------------
    {
      deactivate_speculative();
      // Remove our reference to the physical region
      region = PhysicalRegion();
      privilege_path = RegionTreePath();
#ifdef DEBUG_HIGH_LEVEL
      mapping_path = RegionTreePath();
#endif
      fields.clear();
      grants.clear();
      wait_barriers.clear();
      arrive_barriers.clear();
      // Return this operation to the runtime
      runtime->free_release_op(this);
    }

    //--------------------------------------------------------------------------
    const char* ReleaseOp::get_logging_name(void)
    //--------------------------------------------------------------------------
    {
      return "Release";
    }

    //--------------------------------------------------------------------------
    void ReleaseOp::trigger_dependence_analysis(void)
    //--------------------------------------------------------------------------
    {
      begin_dependence_analysis();
      // Register a dependence on our predicate
      register_predicate_dependence();
      // First register any mapping dependences that we have
      runtime->forest->perform_dependence_analysis(parent_ctx->get_context(),
                                                   this, 0/*idx*/, requirement,
                                                   privilege_path);
      // Now tell the forest that we are relinquishing user-level coherence
      runtime->forest->release_user_coherence(parent_ctx->get_context(),
                                              requirement.region,
                                              requirement.privilege_fields);
      end_dependence_analysis();
    }

    //--------------------------------------------------------------------------
    void ReleaseOp::resolve_true(void)
    //--------------------------------------------------------------------------
    {
      // Put this on the queue of stuff to do
      runtime->add_to_local_queue(parent_ctx->get_executing_processor(),
                                  this, false/*prev fail*/);
    }

    //--------------------------------------------------------------------------
    void ReleaseOp::resolve_false(void)
    //--------------------------------------------------------------------------
    {
      // Clean up this operation
      complete_execution();
      complete_mapping();
    }

    //--------------------------------------------------------------------------
    bool ReleaseOp::speculate(bool &value)
    //--------------------------------------------------------------------------
    {
      Processor exec_proc = parent_ctx->get_executing_processor();
      return runtime->invoke_mapper_speculate(exec_proc, this, value);
    }

    //--------------------------------------------------------------------------
    bool ReleaseOp::trigger_execution(void)
    //--------------------------------------------------------------------------
    {
      RegionTreeContext physical_ctx = 
        parent_ctx->find_enclosing_physical_context(requirement.parent);
      Processor local_proc = parent_ctx->get_executing_processor();
      // If we haven't already premapped the path, then do so now
      if (!requirement.premapped)
      {
        // Use our parent_ctx as the mappable since technically
        // we aren't a mappable.  Technically this shouldn't do anything
        // because we've marked ourselves as being restricted.
        requirement.premapped = runtime->forest->premap_physical_region(
                  physical_ctx, privilege_path, requirement, 
                  this, parent_ctx, local_proc
#ifdef DEBUG_HIGH_LEVEL
                  , 0/*idx*/, get_logging_name(), unique_op_id
#endif
                  );
      }
      // If we couldn't premap, then we need to try again later
      if (!requirement.premapped)
        return false;
      // Now all we need to do is close the physical context to
      // the logical region that we are releasing.  Since we
      // are read-write-exclusive, it will invalidate all other
      // physical instances in the region tree.
      Event release_event = 
        runtime->forest->close_physical_context(physical_ctx,
                                                requirement,
                                                this, parent_ctx,
                                                local_proc,
                                                region.impl->get_reference()
#ifdef DEBUG_HIGH_LEVEL
                                                , 0/*idx*/
                                                , get_logging_name()
                                                , unique_op_id
#endif
                                                );
      std::set<Event> release_preconditions;
      release_preconditions.insert(release_event);
      if (!wait_barriers.empty())
      {
        for (std::vector<PhaseBarrier>::const_iterator it = 
              wait_barriers.begin(); it != wait_barriers.end(); it++)
        {
          Event e = it->phase_barrier.get_previous_phase();
          release_preconditions.insert(e);
        }
      }
      if (!grants.empty())
      {
        for (std::vector<Grant>::const_iterator it = grants.begin();
              it != grants.end(); it++)
        {
          Event e = it->impl->acquire_grant();
          release_preconditions.insert(e);
        }
      }
      Event release_complete = Event::merge_events(release_preconditions);
      
      // Chain any arrival barriers
      if (!arrive_barriers.empty())
      {
        for (std::vector<PhaseBarrier>::const_iterator it = 
              arrive_barriers.begin(); it != arrive_barriers.end(); it++)
        {
          it->phase_barrier.arrive(1/*count*/, release_complete);
        }
      }
      
      // Mark that we completed mapping
      complete_mapping();

      // See if we already triggered
      if (!release_complete.has_triggered())
      {
        completion_event.trigger(release_complete);
        need_completion_trigger = false;
#ifdef SPECIALIZED_UTIL_PROCS
        Processor util = runtime->get_cleanup_proc(local_proc);
#else
        Processor util = runtime->find_utility_group();
#endif
        DeferredCompleteArgs deferred_complete_args;
        deferred_complete_args.hlr_id = HLR_DEFERRED_COMPLETE_ID;
        deferred_complete_args.proxy_this = this;
        util.spawn(HLR_TASK_ID, &deferred_complete_args,
                   sizeof(deferred_complete_args), release_complete);
      }
      else
        deferred_complete();
      // We succeeded in mapping
      return true;
    }

    //--------------------------------------------------------------------------
    void ReleaseOp::deferred_complete(void)
    //--------------------------------------------------------------------------
    {
      // Mark that we're done executing
      complete_execution();
    }

    //--------------------------------------------------------------------------
    Mappable::MappableKind ReleaseOp::get_mappable_kind(void) const
    //--------------------------------------------------------------------------
    {
      return RELEASE_MAPPABLE;
    }

    //--------------------------------------------------------------------------
    Task* ReleaseOp::as_mappable_task(void) const
    //--------------------------------------------------------------------------
    {
      return NULL;
    }

    //--------------------------------------------------------------------------
    Copy* ReleaseOp::as_mappable_copy(void) const
    //--------------------------------------------------------------------------
    {
      return NULL;
    }

    //--------------------------------------------------------------------------
    Inline* ReleaseOp::as_mappable_inline(void) const
    //--------------------------------------------------------------------------
    {
      return NULL;
    }

    //--------------------------------------------------------------------------
    Acquire* ReleaseOp::as_mappable_acquire(void) const
    //--------------------------------------------------------------------------
    {
      return NULL;
    }

    //--------------------------------------------------------------------------
    Release* ReleaseOp::as_mappable_release(void) const
    //--------------------------------------------------------------------------
    {
      ReleaseOp *proxy_this = const_cast<ReleaseOp*>(this);
      return proxy_this;
    }

    //--------------------------------------------------------------------------
    UniqueID ReleaseOp::get_unique_mappable_id(void) const
    //--------------------------------------------------------------------------
    {
      return unique_op_id;
    }

    //--------------------------------------------------------------------------
    const RegionRequirement& ReleaseOp::get_requirement(void) const
    //--------------------------------------------------------------------------
    {
      return requirement;
    }

    //--------------------------------------------------------------------------
    void ReleaseOp::check_release_privilege(void)
    //--------------------------------------------------------------------------
    {
      // Check to make sure the physical region was mapped for
      // the same logical region.
      {
        const RegionRequirement &req = region.impl->get_requirement();
        if (req.region != requirement.region)
        {
          log_region(LEVEL_ERROR,"Mismatch between logical region (" IDFMT 
                                 ",%d,%d) and logical region (" IDFMT ",%d,%d) "
                                 "used for mapping physical region for release "
                                 "operation (ID %lld)",
                                 requirement.region.index_space.id,
                                 requirement.region.field_space.id,
                                 requirement.region.tree_id,
                                 req.region.index_space.id,
                                 req.region.field_space.id,
                                 req.region.tree_id, unique_op_id);
#ifdef DEBUG_HIGH_LEVEL
          assert(false);
#endif
          exit(ERROR_RELEASE_MISMATCH);
        }
      }
      FieldID bad_field;
      LegionErrorType et = runtime->verify_requirement(requirement, bad_field);
      // If that worked, check the privileges, but only check the
      // data and not the actual privilege values since we're
      // using psuedo-read-write-exclusive
      if (et == NO_ERROR)
        et = parent_ctx->check_privilege(requirement, bad_field, true/*skip*/);
      switch (et)
      {
        case NO_ERROR:
          break;
        case ERROR_INVALID_REGION_HANDLE:
          {
            log_region(LEVEL_ERROR,"Requirest for invalid region handle "
                                   "(" IDFMT ",%d,%d) of requirement for "
                                   "release operation (ID %lld)",
                                   requirement.region.index_space.id, 
                                   requirement.region.field_space.id, 
                                   requirement.region.tree_id, 
                                   unique_op_id);
#ifdef DEBUG_HIGH_LEVEL
            assert(false);
#endif
            exit(ERROR_INVALID_REGION_HANDLE);
          }
        case ERROR_FIELD_SPACE_FIELD_MISMATCH:
          {
            FieldSpace sp = (requirement.handle_type == SINGULAR) || 
                            (requirement.handle_type == REG_PROJECTION)
                             ? requirement.region.field_space : 
                               requirement.partition.field_space;
            log_region(LEVEL_ERROR,"Field %d is not a valid field of field "
                                   "space %d of requirement for release "
                                   "operation (ID %lld)",
                                   bad_field, sp.id, unique_op_id);
#ifdef DEBUG_HIGH_LEVEL
            assert(false);
#endif
            exit(ERROR_FIELD_SPACE_FIELD_MISMATCH);
          }
        case ERROR_BAD_PARENT_REGION:
          {
            log_region(LEVEL_ERROR,"Parent task %s (ID %lld) of release "
                                   "operation (ID %lld) does not have a region "
                                   "requirement for region (" IDFMT 
                                   ",%x,%x) as a parent",
                                   parent_ctx->variants->name, 
                                   parent_ctx->get_unique_task_id(),
                                   unique_op_id, 
                                   requirement.region.index_space.id,
                                   requirement.region.field_space.id, 
                                   requirement.region.tree_id);
#ifdef DEBUG_HIGH_LEVEL
            assert(false);
#endif
            exit(ERROR_BAD_PARENT_REGION);
          }
        case ERROR_BAD_REGION_PATH:
          {
            log_region(LEVEL_ERROR,"Region (" IDFMT ",%x,%x) is not a "
                                   "sub-region of parent region (" IDFMT 
                                   ",%x,%x) of requirement for release "
                                   "operation (ID %lld)",
                                   requirement.region.index_space.id,
                                   requirement.region.field_space.id, 
                                   requirement.region.tree_id,
                                   requirement.parent.index_space.id,
                                   requirement.parent.field_space.id,
                                   requirement.parent.tree_id, unique_op_id);
#ifdef DEBUG_HIGH_LEVEL
            assert(false);
#endif
            exit(ERROR_BAD_REGION_PATH);
          }
        case ERROR_BAD_REGION_TYPE:
          {
            log_region(LEVEL_ERROR,"Region requirement of release operation "
                                   "(ID %lld) cannot find privileges for field "
                                   "%d in parent task",
                                   unique_op_id, bad_field);
#ifdef DEBUG_HIGH_LEVEL
            assert(false);
#endif
            exit(ERROR_BAD_REGION_TYPE);
          }
        // these should never happen with an release operation 
        case ERROR_INVALID_INSTANCE_FIELD:
        case ERROR_DUPLICATE_INSTANCE_FIELD:
        case ERROR_BAD_REGION_PRIVILEGES:
        case ERROR_NON_DISJOINT_PARTITION: 
        default:
          assert(false); // Should never happen
      }
    }

    /////////////////////////////////////////////////////////////
    // Future Predicate Operation
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    FuturePredOp::FuturePredOp(Runtime *rt)
      : PredicateOp(rt)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    FuturePredOp::FuturePredOp(const FuturePredOp &rhs)
      : PredicateOp(NULL)
    //--------------------------------------------------------------------------
    {
      // should never happen
      assert(false);
    }

    //--------------------------------------------------------------------------
    FuturePredOp::~FuturePredOp(void)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    FuturePredOp& FuturePredOp::operator=(const FuturePredOp &rhs)
    //--------------------------------------------------------------------------
    {
      // should never be called
      assert(false);
      return *this;
    }

    //--------------------------------------------------------------------------
    void FuturePredOp::activate(void)
    //--------------------------------------------------------------------------
    {
      activate_predicate();
    }

    //--------------------------------------------------------------------------
    void FuturePredOp::deactivate(void)
    //--------------------------------------------------------------------------
    {
      deactivate_predicate();
      future = Future();
      runtime->free_future_predicate_op(this);
    }

    //--------------------------------------------------------------------------
    const char* FuturePredOp::get_logging_name(void)
    //--------------------------------------------------------------------------
    {
      return "Future Predicate";
    }

    //--------------------------------------------------------------------------
    void FuturePredOp::initialize(SingleTask *ctx, Future f)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_HIGH_LEVEL
      assert(ctx != NULL);
      assert(f.impl != NULL);
#endif
      // Don't track this as it can lead to deadlock because
      // predicates can't complete until all their references from
      // the parent task have been removed.
      initialize_operation(ctx, false/*track*/, Event::NO_EVENT);
      future = f;
    }

    //--------------------------------------------------------------------------
    void FuturePredOp::resolve_future_predicate(void)
    //--------------------------------------------------------------------------
    {
      bool valid;
      bool value = future.impl->get_boolean_value(valid);
#ifdef DEBUG_HIGH_LEVEL
      assert(valid);
#endif
      set_resolved_value(get_generation(), value);
    }

    //--------------------------------------------------------------------------
    void FuturePredOp::trigger_dependence_analysis(void)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_HIGH_LEVEL
      assert(future.impl->task != NULL);
#endif
      begin_dependence_analysis();
      // Register this operation as dependent on task that
      // generated the future
      register_dependence(future.impl->task, future.impl->task_gen);
      end_dependence_analysis();
    }

    //--------------------------------------------------------------------------
    void FuturePredOp::trigger_mapping(void)
    //--------------------------------------------------------------------------
    {
      // See if we have a value
      bool valid;
      bool value = future.impl->get_boolean_value(valid);
      if (valid)
        set_resolved_value(get_generation(), value);
      else
      {
        // Launch a task to get the value
        add_predicate_reference();
        ResolveFuturePredArgs args;
        args.hlr_id = HLR_RESOLVE_FUTURE_PRED_ID;
        args.future_pred_op = this;
        Processor util_proc = runtime->find_utility_group();
        util_proc.spawn(HLR_TASK_ID, &args, sizeof(args),
                        future.impl->get_ready_event());
      }
      // Mark that we completed mapping this operation
      complete_mapping();
    } 

    /////////////////////////////////////////////////////////////
    // Not Predicate Operation
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    NotPredOp::NotPredOp(Runtime *rt)
      : PredicateOp(rt)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    NotPredOp::NotPredOp(const NotPredOp &rhs)
      : PredicateOp(NULL)
    //--------------------------------------------------------------------------
    {
      // should never happen
      assert(false);
    }

    //--------------------------------------------------------------------------
    NotPredOp::~NotPredOp(void)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    NotPredOp& NotPredOp::operator=(const NotPredOp &rhs)
    //--------------------------------------------------------------------------
    {
      // should never be called
      assert(false);
      return *this;
    }

    //--------------------------------------------------------------------------
    void NotPredOp::initialize(SingleTask *ctx, const Predicate &p)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_HIGH_LEVEL
      assert(ctx != NULL);
#endif
      // Don't track this as it can lead to deadlock because
      // predicates can't complete until all their references from
      // the parent task have been removed.
      initialize_operation(ctx, false/*track*/, Event::NO_EVENT);
      // Don't forget to reverse the values
      if (p == Predicate::TRUE_PRED)
        set_resolved_value(get_generation(), false);
      else if (p == Predicate::FALSE_PRED)
        set_resolved_value(get_generation(), true);
      else
      {
#ifdef DEBUG_HIGH_LEVEL
        assert(p.impl != NULL);
#endif
        pred_op = p.impl;
        pred_op->add_predicate_reference();
      }
    }

    //--------------------------------------------------------------------------
    void NotPredOp::activate(void)
    //--------------------------------------------------------------------------
    {
      activate_predicate();
      pred_op = NULL;
    }

    //--------------------------------------------------------------------------
    void NotPredOp::deactivate(void)
    //--------------------------------------------------------------------------
    {
      deactivate_predicate();
      runtime->free_not_predicate_op(this);
    }

    //--------------------------------------------------------------------------
    const char* NotPredOp::get_logging_name(void)
    //--------------------------------------------------------------------------
    {
      return "Not Predicate";
    }

    //--------------------------------------------------------------------------
    void NotPredOp::trigger_dependence_analysis(void)
    //--------------------------------------------------------------------------
    {
      begin_dependence_analysis();
      if (pred_op != NULL)
        register_dependence(pred_op, pred_op->get_generation());
      end_dependence_analysis();
    }

    //--------------------------------------------------------------------------
    void NotPredOp::trigger_mapping(void)
    //--------------------------------------------------------------------------
    {
      if (pred_op != NULL)
      {
        bool prev_value;
        bool valid = pred_op->register_waiter(this, get_generation(),
                                              prev_value);
        // Don't forget to negate 
        if (valid)
          set_resolved_value(get_generation(), !prev_value);
        // Now we can remove the reference we added
        pred_op->remove_predicate_reference();
      }
      complete_mapping();
    }

    //--------------------------------------------------------------------------
    void NotPredOp::notify_predicate_value(GenerationID prev_gen, bool value)
    //--------------------------------------------------------------------------
    {
      // No short circuit in this one
      // We can test this without the lock because 
      // it is monotonically increasing
#ifdef DEBUG_HIGH_LEVEL
      assert(prev_gen == get_generation());
#endif
      // Don't forget to negate the value
      set_resolved_value(prev_gen, !value);
    }

    /////////////////////////////////////////////////////////////
    // And Predicate Operation
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    AndPredOp::AndPredOp(Runtime *rt)
      : PredicateOp(rt)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    AndPredOp::AndPredOp(const AndPredOp &rhs)
      : PredicateOp(NULL)
    //--------------------------------------------------------------------------
    {
      // should never happen
      assert(false);
    }

    //--------------------------------------------------------------------------
    AndPredOp::~AndPredOp(void)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    AndPredOp& AndPredOp::operator=(const AndPredOp &rhs)
    //--------------------------------------------------------------------------
    {
      // should never be called
      assert(false);
      return *this;
    }

    //--------------------------------------------------------------------------
    void AndPredOp::initialize(SingleTask *ctx,
                               const Predicate &p1, const Predicate &p2)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_HIGH_LEVEL
      assert(ctx != NULL);
#endif
      // Don't track this as it can lead to deadlock because
      // predicates can't complete until all their references from
      // the parent task have been removed.
      initialize_operation(ctx, false/*track*/, Event::NO_EVENT);
      // Short circuit case
      if ((p1 == Predicate::FALSE_PRED) || (p2 == Predicate::FALSE_PRED))
      {
        set_resolved_value(get_generation(), false);
        return;
      }
      if (p1 == Predicate::TRUE_PRED)
      {
        left_value = true;
        left_valid = true;
      }
      else
      {
#ifdef DEBUG_HIGH_LEVEL
        assert(p1.impl != NULL);
#endif
        left_valid = false;
        left = p1.impl;
        left->add_predicate_reference();
      }
      if (p2 == Predicate::TRUE_PRED)
      {
        right_value = true;
        right_valid = true;
      }
      else
      {
#ifdef DEBUG_HIGH_LEVEL
        assert(p2.impl != NULL);
#endif
        right_valid = false;
        right = p2.impl;
        right->add_predicate_reference();
      }
    }

    //--------------------------------------------------------------------------
    void AndPredOp::activate(void)
    //--------------------------------------------------------------------------
    {
      activate_predicate();
      left = NULL;
      right = NULL;
      left_valid = false;
      right_valid = false;
    }

    //--------------------------------------------------------------------------
    void AndPredOp::deactivate(void)
    //--------------------------------------------------------------------------
    {
      deactivate_predicate();
      runtime->free_and_predicate_op(this);
    }

    //--------------------------------------------------------------------------
    const char* AndPredOp::get_logging_name(void)
    //--------------------------------------------------------------------------
    {
      return "And Predicate";
    }

    //--------------------------------------------------------------------------
    void AndPredOp::trigger_dependence_analysis(void)
    //--------------------------------------------------------------------------
    {
      begin_dependence_analysis();
      if (left != NULL)
        register_dependence(left, left->get_generation());
      if (left != NULL)
        register_dependence(right, right->get_generation());
      end_dependence_analysis();
    }

    //--------------------------------------------------------------------------
    void AndPredOp::trigger_mapping(void)
    //--------------------------------------------------------------------------
    {
      // Hold the lock when doing this to prevent 
      // any triggers from interfering with the analysis
      bool need_resolve = false;
      bool resolve_value;
      GenerationID local_gen = get_generation();
      {
        AutoLock o_lock(op_lock);
        if (!predicate_resolved)
        {
          if (left != NULL)
            left_valid = left->register_waiter(this, get_generation(),
                                               left_value);
          if (right != NULL)
            right_valid = right->register_waiter(this, get_generation(),
                                                 right_value);
          // Both valid
          if (left_valid && right_valid)
          {
            need_resolve = true;
            resolve_value = (left_value && right_value);
          }
          // Left short circuit
          else if (left_valid && !left_value)
          {
            need_resolve = true;
            resolve_value = false;
          }
          // Right short circuit
          else if (right_valid && !right_value) 
          {
            need_resolve = true;
            resolve_value = false;
          }
        }
      }
      if (need_resolve)
        set_resolved_value(local_gen, resolve_value);
      // Clean up any references that we have
      if (left != NULL)
        left->remove_predicate_reference();
      if (right != NULL)
        right->remove_predicate_reference();
      complete_mapping();
    }

    //--------------------------------------------------------------------------
    void AndPredOp::notify_predicate_value(GenerationID pred_gen, bool value)
    //--------------------------------------------------------------------------
    {
      bool need_resolve, resolve_value;
      if (pred_gen == get_generation())
      {
        AutoLock o_lock(op_lock);
        // Check again to make sure we didn't lose the race
        if ((pred_gen == get_generation()) && !predicate_resolved)
        {
          if (!value)
          {
            need_resolve = true;
            resolve_value = false;
          }
          else
          {
            // Figure out which of the two values to fill in
#ifdef DEBUG_HIGH_LEVEL
            assert(!left_valid || !right_valid);
#endif
            if (!left_valid)
            {
              left_value = value;
              left_valid = true;
            }
            else
            {
              right_value = value;
              right_valid = true;
            }
            if (left_valid && right_valid)
            {
              need_resolve = true;
              resolve_value = (left_value && right_value);
            }
          }
        }
        else
          need_resolve = false;
      }
      else
        need_resolve = false;
      if (need_resolve)
        set_resolved_value(pred_gen, resolve_value);
    }

    /////////////////////////////////////////////////////////////
    // Or Predicate Operation
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    OrPredOp::OrPredOp(Runtime *rt)
      : PredicateOp(rt)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    OrPredOp::OrPredOp(const OrPredOp &rhs)
      : PredicateOp(NULL)
    //--------------------------------------------------------------------------
    {
      // should never happen
      assert(false);
    }

    //--------------------------------------------------------------------------
    OrPredOp::~OrPredOp(void)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    OrPredOp& OrPredOp::operator=(const OrPredOp &rhs)
    //--------------------------------------------------------------------------
    {
      // should never be called
      assert(false);
      return *this;
    }

    //--------------------------------------------------------------------------
    void OrPredOp::initialize(SingleTask *ctx,
                              const Predicate &p1, const Predicate &p2)
    //--------------------------------------------------------------------------
    {
#ifdef DEBUG_HIGH_LEVEL
      assert(ctx != NULL);
#endif
      // Don't track this as it can lead to deadlock because
      // predicates can't complete until all their references from
      // the parent task have been removed.
      initialize_operation(ctx, false/*track*/, Event::NO_EVENT);
      // Short circuit case
      if ((p1 == Predicate::TRUE_PRED) || (p2 == Predicate::TRUE_PRED))
      {
        set_resolved_value(get_generation(), true);
        return;
      }
      if (p1 == Predicate::FALSE_PRED)
      {
        left_value = false;
        left_valid = true;
      }
      else
      {
        left = p1.impl;
        left_valid = false;
        left->add_predicate_reference();
      }
      if (p2 == Predicate::FALSE_PRED)
      {
        right_value = false;
        right_valid = true;
      }
      else
      {
        right = p2.impl;
        right_valid = false;
        right->add_predicate_reference();
      }
    }

    //--------------------------------------------------------------------------
    void OrPredOp::activate(void)
    //--------------------------------------------------------------------------
    {
      activate_predicate();
      left = NULL;
      right = NULL;
      left_valid = false;
      right_valid = false;
    }

    //--------------------------------------------------------------------------
    void OrPredOp::deactivate(void)
    //--------------------------------------------------------------------------
    {
      deactivate_predicate();
      runtime->free_or_predicate_op(this);
    }

    //--------------------------------------------------------------------------
    const char* OrPredOp::get_logging_name(void)
    //--------------------------------------------------------------------------
    {
      return "Or Predicate";
    }

    //--------------------------------------------------------------------------
    void OrPredOp::trigger_dependence_analysis(void)
    //--------------------------------------------------------------------------
    {
      begin_dependence_analysis();
      if (left != NULL)
        register_dependence(left, left->get_generation());
      if (right != NULL)
        register_dependence(right, right->get_generation());
      end_dependence_analysis();
    }

    //--------------------------------------------------------------------------
    void OrPredOp::trigger_mapping(void)
    //--------------------------------------------------------------------------
    {
      // Hold the lock when doing this to prevent 
      // any triggers from interfering with the analysis
      bool need_resolve = false;
      bool resolve_value;
      GenerationID local_gen = get_generation();
      {
        AutoLock o_lock(op_lock);
        if (!predicate_resolved)
        {
          if (left != NULL)
            left_valid = left->register_waiter(this, get_generation(),
                                               left_value);
          if (right != NULL)
            right_valid = right->register_waiter(this, get_generation(),
                                                 right_value);
          // Both valid
          if (left_valid && right_valid)
          {
            need_resolve = true;
            resolve_value = (left_value || right_value);
          }
          // Left short circuit
          else if (left_valid && left_value)
          {
            need_resolve = true;
            resolve_value = true;
          }
          // Right short circuit
          else if (right_valid && right_value) 
          {
            need_resolve = true;
            resolve_value = true;
          }
        }
      }
      if (need_resolve)
        set_resolved_value(local_gen, resolve_value);
      // Clean up any references that we have
      if (left != NULL)
        left->remove_predicate_reference();
      if (right != NULL)
        right->remove_predicate_reference();
      complete_mapping();
    }

    //--------------------------------------------------------------------------
    void OrPredOp::notify_predicate_value(GenerationID pred_gen, bool value)
    //--------------------------------------------------------------------------
    {
      bool need_resolve, resolve_value;
      if (pred_gen == get_generation())
      {
        AutoLock o_lock(op_lock);
        // Check again to make sure we didn't lose the race
        if ((pred_gen == get_generation()) && !predicate_resolved)
        {
          if (value)
          {
            need_resolve = true;
            resolve_value = true;
          }
          else
          {
            // Figure out which of the two values to fill in
#ifdef DEBUG_HIGH_LEVEL
            assert(!left_valid || !right_valid);
#endif
            if (!left_valid)
            {
              left_value = value;
              left_valid = true;
            }
            else
            {
              right_value = value;
              right_valid = true;
            }
            if (left_valid && right_valid)
            {
              need_resolve = true;
              resolve_value = (left_value || right_value);
            }
          }
        }
        else
          need_resolve = false;
      }
      else
        need_resolve = false;
      if (need_resolve)
        set_resolved_value(pred_gen, resolve_value);
    }


    /////////////////////////////////////////////////////////////
    // Must Epoch Operation 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    MustEpochOp::MustEpochOp(Runtime *rt)
      : Operation(rt)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    MustEpochOp::MustEpochOp(const MustEpochOp &rhs)
      : Operation(NULL)
    //--------------------------------------------------------------------------
    {
      // should never be called
      assert(false);
    }

    //--------------------------------------------------------------------------
    MustEpochOp::~MustEpochOp(void)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    MustEpochOp& MustEpochOp::operator=(const MustEpochOp &rhs)
    //--------------------------------------------------------------------------
    {
      // should never be called
      assert(false);
      return *this;
    }

    //--------------------------------------------------------------------------
    FutureMap MustEpochOp::initialize(SingleTask *ctx,
                                      const MustEpochLauncher &launcher,
                                      bool check_privileges)
    //--------------------------------------------------------------------------
    {
      // Initialize this operation
      // Make an event for the all children mapped that is a merged of
      // all our tasks all children mapped events
      UserEvent all_children_mapped = UserEvent::create_user_event();
      initialize_operation(ctx, true/*track*/, all_children_mapped);
      // Initialize operations for everything in the launcher
      // Note that we do not track these operations as we want them all to
      // appear as a single operation to the parent context in order to
      // avoid deadlock with the maximum window size.
      indiv_tasks.resize(launcher.single_tasks.size());
      for (unsigned idx = 0; idx < launcher.single_tasks.size(); idx++)
      {
        indiv_tasks[idx] = runtime->get_available_individual_task();
        indiv_tasks[idx]->initialize_task(ctx, launcher.single_tasks[idx],
                                          check_privileges, false/*track*/);
        indiv_tasks[idx]->set_must_epoch(this, idx);
        // If we have a trace, set it for this operation as well
        if (trace != NULL)
          indiv_tasks[idx]->set_trace(trace);
      }
      indiv_triggered.resize(indiv_tasks.size(), false);
      for (unsigned idx = 0; idx < launcher.index_tasks.size(); idx++)
      {
        index_tasks[idx] = runtime->get_available_index_task();
        index_tasks[idx]->initialize_task(ctx, launcher.index_tasks[idx],
                                          check_privileges, false/*track*/);
        index_tasks[idx]->set_must_epoch(this, indiv_tasks.size()+idx);
        if (trace != NULL)
          index_tasks[idx]->set_trace(trace);
      }
      index_triggered.resize(index_tasks.size(), false);
      mapper_id = launcher.map_id;
      mapper_tag = launcher.mapping_tag;
      // Make a new future map for storing our results
      // We'll fill it in later
      result_map = legion_new<FutureMap::Impl>(ctx, 
                                               get_completion_event(), runtime);
      // Once all the tasks have been initialized we can defer
      // our all mapped event on all their all mapped events
      std::set<Event> tasks_all_mapped;
      for (std::vector<IndividualTask*>::const_iterator it = 
            indiv_tasks.begin(); it != indiv_tasks.end(); it++)
      {
        tasks_all_mapped.insert((*it)->get_children_mapped());
      }
      for (std::vector<IndexTask*>::const_iterator it = 
            index_tasks.begin(); it != index_tasks.end(); it++)
      {
        tasks_all_mapped.insert((*it)->get_children_mapped());
      }
      all_children_mapped.trigger(Event::merge_events(tasks_all_mapped));
#ifdef DEBUG_HIGH_LEVEL
      for (unsigned idx = 0; idx < indiv_tasks.size(); idx++)
      {
        result_map.impl->add_valid_point(indiv_tasks[idx]->index_point);
      }
      for (unsigned idx = 0; idx < index_tasks.size(); idx++)
      {
        result_map.impl->add_valid_domain(index_tasks[idx]->index_domain);
      }
#endif
      return result_map;
    }

    //--------------------------------------------------------------------------
    void MustEpochOp::set_task_options(ProcessorManager *manager)
    //--------------------------------------------------------------------------
    {
      // Mark that all these operations will map locally because we
      // have to do a single mapping call for all our operations.
      for (unsigned idx = 0; idx < indiv_tasks.size(); idx++)
      {
        manager->invoke_mapper_set_task_options(indiv_tasks[idx]);
        indiv_tasks[idx]->map_locally = true;
      }
      for (unsigned idx = 0; idx < index_tasks.size(); idx++)
      {
        manager->invoke_mapper_set_task_options(index_tasks[idx]);
        index_tasks[idx]->map_locally = true;
      }
    }

    //--------------------------------------------------------------------------
    void MustEpochOp::find_conflicted_regions(
                                         std::vector<PhysicalRegion> &conflicts)
    //--------------------------------------------------------------------------
    {
      // Dump them all into a set when they are done to deduplicate them
      // This is not the most optimized way to do this, but it will work for now
      std::set<PhysicalRegion> temp_conflicts;
      for (unsigned idx = 0; idx < indiv_tasks.size(); idx++)
      {
        std::vector<PhysicalRegion> temp;
        parent_ctx->find_conflicting_regions(indiv_tasks[idx], temp);
        temp_conflicts.insert(temp.begin(),temp.end());
      }
      for (unsigned idx = 0; idx < index_tasks.size(); idx++)
      {
        std::vector<PhysicalRegion> temp;
        parent_ctx->find_conflicting_regions(index_tasks[idx], temp);
        temp_conflicts.insert(temp.begin(),temp.end());
      }
      conflicts.insert(conflicts.end(),
                       temp_conflicts.begin(),temp_conflicts.end());
    }

    //--------------------------------------------------------------------------
    void MustEpochOp::activate(void)
    //--------------------------------------------------------------------------
    {
      activate_operation();
      mapper_id = 0;
      mapper_tag = 0;
      // Set to 1 to include the triggers we get for our operation
      remaining_subop_completes = 1;
      remaining_subop_commits = 1;
      triggering_complete = false;
    }

    //--------------------------------------------------------------------------
    void MustEpochOp::deactivate(void)
    //--------------------------------------------------------------------------
    {
      deactivate_operation();
      // All the sub-operations we have will deactivate themselves
      indiv_tasks.clear();
      indiv_triggered.clear();
      index_tasks.clear();
      index_triggered.clear();
      slice_tasks.clear();
      single_tasks.clear();
      // Remove our reference on the future map
      result_map = FutureMap();
      constraints.clear();
      task_sets.clear();
      dependences.clear();
      // Return this operation to the free list
      runtime->free_epoch_op(this);
    }

    //--------------------------------------------------------------------------
    const char* MustEpochOp::get_logging_name(void)
    //--------------------------------------------------------------------------
    {
      return "Must Epoch";
    }

    //--------------------------------------------------------------------------
    void MustEpochOp::trigger_dependence_analysis(void)
    //--------------------------------------------------------------------------
    {
      begin_dependence_analysis();
      // For every one of our sub-operations, add an additional mapping 
      // dependence.  When our sub-operations map, they will trigger these
      // mapping dependences which guarantees that we will not be able to
      // map until all of the sub-operations are ready to map.
      outstanding_mapping_deps += (indiv_tasks.size() + index_tasks.size());
      for (unsigned idx = 0; idx < indiv_tasks.size(); idx++)
        indiv_tasks[idx]->trigger_dependence_analysis();
      for (unsigned idx = 0; idx < index_tasks.size(); idx++)
        index_tasks[idx]->trigger_dependence_analysis();
      end_dependence_analysis();
    }

    //--------------------------------------------------------------------------
    bool MustEpochOp::trigger_execution(void)
    //--------------------------------------------------------------------------
    {
      // Call trigger execution on each of our sub-operations, since they
      // each have marked that they have a must_epoch owner, they will
      // not actually map and launch, but instead will register all the base
      // operations with us.  Note this step requires that we mark everything
      // as needing to locally map in the 'initialize' method.  Check for
      // error codes indicating failed pre-mapping.
      if (!triggering_complete)
      {
        task_sets.resize(indiv_tasks.size()+index_tasks.size());
        MustEpochTriggerer triggerer(this);
        if (!triggerer.trigger_tasks(indiv_tasks, indiv_triggered,
                                     index_tasks, index_triggered))
          return false;

#ifdef DEBUG_HIGH_LEVEL
        assert(!single_tasks.empty());
#endif 

        // Next build the set of single tasks and all their constraints.
        // Iterate over all the recorded dependences
        for (std::vector<DependenceRecord>::const_iterator it = 
              dependences.begin(); it != dependences.end(); it++)
        {
          // Add constraints for all the different elements
          const std::set<SingleTask*> &s1 = task_sets[it->op1_idx];
          const std::set<SingleTask*> &s2 = task_sets[it->op2_idx];
          for (std::set<SingleTask*>::const_iterator it1 = s1.begin();
                it1 != s1.end(); it1++)
          {
            for (std::set<SingleTask*>::const_iterator it2 = s2.begin();
                  it2 != s2.end(); it2++)
            {
              constraints.push_back(Mapper::MappingConstraint(
                                                       *it1, it->reg1_idx,
                                                       *it2, it->reg2_idx,
                                                       it->dtype));
            }
          }
        }
        // Mark that we have finished building all the constraints so
        // we don't have to redo it if we end up failing a mapping.
        triggering_complete = true;
      }
      // Next make the mapper call to perform the mapping for all the tasks
      std::vector<Task*> copy_single_tasks(single_tasks.begin(),
                                           single_tasks.end());
      Processor mapper_proc = parent_ctx->get_executing_processor();
      bool notify = runtime->invoke_mapper_map_must_epoch(
          mapper_proc, copy_single_tasks, constraints, mapper_id, mapper_tag);

      // Check that all the tasks have been assigned to different processors
      std::map<Processor,SingleTask*> target_procs;
      for (std::set<SingleTask*>::const_iterator it = 
            single_tasks.begin(); it != single_tasks.end(); it++)
      {
        if (target_procs.find((*it)->target_proc) != target_procs.end())
        {
          SingleTask *other = target_procs[(*it)->target_proc];
          log_run(LEVEL_ERROR,"MUST EPOCH ERROR: Task %s (ID %lld) and "
              "task %s (ID %lld) both requested to be run on processor "
              IDFMT "!",
              (*it)->variants->name, (*it)->get_unique_task_id(),
              other->variants->name, other->get_unique_task_id(),
              other->target_proc.id);
#ifdef DEBUG_HIGH_LEVEL
          assert(false);
#endif
          exit(ERROR_MUST_EPOCH_FAILURE);
        }
        target_procs[(*it)->target_proc] = *it;
      }

      // Then we need to actually perform the mapping
      {
        MustEpochMapper mapper(this); 
        if (!mapper.map_tasks(single_tasks))
          return false;
      }

      // Everybody successfully mapped so now check that all
      // of the constraints have been satisfied
      for (std::vector<Mapper::MappingConstraint>::const_iterator it = 
            constraints.begin(); it != constraints.end(); it++)
      {
        // We know that all these tasks are single tasks
        // so doing static casts are safe
        SingleTask *t1 = static_cast<SingleTask*>(const_cast<Task*>(it->t1));
        SingleTask *t2 = static_cast<SingleTask*>(const_cast<Task*>(it->t2));
        PhysicalManager *inst1 = t1->get_instance(it->idx1);
        PhysicalManager *inst2 = t2->get_instance(it->idx2);
        // Check to make sure they selected the same instance 
        if (inst1 != inst2)
        {
          log_run(LEVEL_ERROR,"MUST EPOCH ERROR: failed constraint! "
              "Task %s (ID %lld) mapped region %d to instance " IDFMT " in "
              "memory " IDFMT " , but task %s (ID %lld) mapped region %d to "
              "instance " IDFMT " in memory " IDFMT ".",
              t1->variants->name, t1->get_unique_task_id(), it->idx1,
              inst1->get_instance().id, inst1->memory.id,
              t2->variants->name, t2->get_unique_task_id(), it->idx2,
              inst2->get_instance().id, inst2->memory.id);
#ifdef DEBUG_HIGH_LEVEL
          assert(false);
#endif
          exit(ERROR_MUST_EPOCH_FAILURE);
        }
      }

      // If the mapper wanted to know on success, then tell it
      if (notify)
      {
        // Notify the mappers that the tasks successfully mapped
        for (std::set<SingleTask*>::const_iterator it = 
              single_tasks.begin(); it != single_tasks.end(); it++)
        {
          runtime->invoke_mapper_notify_result(mapper_proc, *it);
        }
      }

      // If we passed all the constraints, then kick everything off
      MustEpochDistributor distributor(this);
      distributor.distribute_tasks(runtime, indiv_tasks, slice_tasks);
      
      // Mark that we are done mapping and executing this operation
      complete_mapping();
      complete_execution();
      return true;
    }

    //--------------------------------------------------------------------------
    void MustEpochOp::trigger_complete(void)
    //--------------------------------------------------------------------------
    {
      bool need_complete;
      {
        AutoLock o_lock(op_lock);
#ifdef DEBUG_HIGH_LEVEL
        assert(remaining_subop_completes > 0);
#endif
        remaining_subop_completes--;
        need_complete = (remaining_subop_completes == 0);
      }
      if (need_complete)
      {
        result_map.impl->complete_all_futures();
        complete_operation();
      }
    }

    //--------------------------------------------------------------------------
    void MustEpochOp::trigger_commit(void)
    //--------------------------------------------------------------------------
    {
      bool need_commit;
      {
        AutoLock o_lock(op_lock);
#ifdef DEBUG_HIGH_LEVEL
        assert(remaining_subop_commits > 0);
#endif
        remaining_subop_commits--;
        need_commit = (remaining_subop_commits == 0);
      }
      if (need_commit)
        commit_operation();
    }

    //--------------------------------------------------------------------------
    void MustEpochOp::verify_dependence(Operation *src_op, GenerationID src_gen,
                                        Operation *dst_op, GenerationID dst_gen)
    //--------------------------------------------------------------------------
    {
      // If they are the same, then we can ignore them
      if ((src_op == dst_op) && (src_gen == dst_gen))
        return;
      // Check to see if the source is one of our operations, if it is
      // then we have an actual dependence which is an error.
      int src_index = find_operation_index(src_op, src_gen);
      if (src_index >= 0)
      {
        int dst_index = find_operation_index(dst_op, dst_gen);
#ifdef DEBUG_HIGH_LEVEL
        assert(dst_index >= 0);
#endif
        TaskOp *src_task = find_task_by_index(src_index);
        TaskOp *dst_task = find_task_by_index(dst_index);
        log_run(LEVEL_ERROR,"MUST EPOCH ERROR: dependence between task "
            "%s (ID %lld) and task %s (ID %lld)\n",
            src_task->variants->name, src_task->get_unique_task_id(),
            dst_task->variants->name, dst_task->get_unique_task_id());
#ifdef DEBUG_HIGH_LEVEL
        assert(false);
#endif
        exit(ERROR_MUST_EPOCH_FAILURE);
      }
    }
    
    //--------------------------------------------------------------------------
    bool MustEpochOp::record_dependence(Operation *src_op, GenerationID src_gen,
                                        Operation *dst_op, GenerationID dst_gen,
                                        unsigned src_idx, unsigned dst_idx,
                                        DependenceType dtype)
    //--------------------------------------------------------------------------
    {
      // If they are the same we can ignore them 
      if ((src_op == dst_op) && (src_gen == dst_gen))
        return true;
      // Check to see if the source is one of our operations
      int src_index = find_operation_index(src_op, src_gen);
      int dst_index = find_operation_index(dst_op, dst_gen);
      if ((src_index >= 0) && (dst_index >= 0))
      {
        // If it is, see what kind of dependence we have
        if ((dtype == TRUE_DEPENDENCE) || (dtype == ANTI_DEPENDENCE) ||
            (dtype == ATOMIC_DEPENDENCE))
        {
          TaskOp *src_task = find_task_by_index(src_index);
          TaskOp *dst_task = find_task_by_index(dst_index);
          log_run(LEVEL_ERROR,"MUST EPOCH ERROR: dependence between region %d "
              "of task %s (ID %lld) and region %d of task %s (ID %lld) of "
              " type %s", src_idx, src_task->variants->name,
              src_task->get_unique_task_id(), dst_idx, 
              dst_task->variants->name, dst_task->get_unique_task_id(),
              (dtype == TRUE_DEPENDENCE) ? "TRUE DEPENDENCE" :
                (dtype == ANTI_DEPENDENCE) ? "ANTI DEPENDENCE" :
                "ATOMIC DEPENDENCE");
#ifdef DEBUG_HIGH_LEVEL
          assert(false);
#endif
          exit(ERROR_MUST_EPOCH_FAILURE);
        }
        else if (dtype == SIMULTANEOUS_DEPENDENCE)
        {
          // Record the dependence kind
          int dst_index = find_operation_index(dst_op, dst_gen);
#ifdef DEBUG_HIGH_LEVEL
          assert(dst_index >= 0);
#endif
          dependences.push_back(DependenceRecord(src_index, dst_index,
                                                 src_idx, dst_idx, dtype)); 
          return false;
        }
        // NO_DEPENDENCE and PROMOTED_DEPENDENCE are not errors
        // and do not need to be recorded
      }
      return true;
    }

    //--------------------------------------------------------------------------
    void MustEpochOp::register_single_task(SingleTask *single, unsigned index)
    //--------------------------------------------------------------------------
    {
      // Can do the first part without the lock 
#ifdef DEBUG_HIGH_LEVEL
      assert(index < task_sets.size());
#endif
      task_sets[index].insert(single);
      AutoLock o_lock(op_lock);
      single_tasks.insert(single);
    }

    //--------------------------------------------------------------------------
    void MustEpochOp::register_slice_task(SliceTask *slice)
    //--------------------------------------------------------------------------
    {
      AutoLock o_lock(op_lock);
      slice_tasks.insert(slice);
    }

    //--------------------------------------------------------------------------
    void MustEpochOp::set_future(const DomainPoint &point, const void *result, 
                                 size_t result_size, bool owner)
    //--------------------------------------------------------------------------
    {
      Future f = result_map.impl->get_future(point);
      f.impl->set_result(result, result_size, owner);
    }

    //--------------------------------------------------------------------------
    void MustEpochOp::unpack_future(const DomainPoint &point, 
                                    Deserializer &derez)
    //--------------------------------------------------------------------------
    {
      Future f = result_map.impl->get_future(point);
      f.impl->unpack_future(derez);
    }

    //--------------------------------------------------------------------------
    void MustEpochOp::register_subop(Operation *op)
    //--------------------------------------------------------------------------
    {
      AutoLock o_lock(op_lock);
      remaining_subop_completes++;
      remaining_subop_commits++;
    }

    //--------------------------------------------------------------------------
    void MustEpochOp::notify_subop_complete(Operation *op)
    //--------------------------------------------------------------------------
    {
      bool need_complete;
      {
        AutoLock o_lock(op_lock);
#ifdef DEBUG_HIGH_LEVEL
        assert(remaining_subop_completes > 0);
#endif
        remaining_subop_completes--;
        need_complete = (remaining_subop_completes == 0);
      }
      if (need_complete)
      {
        // Complete all our futures
        result_map.impl->complete_all_futures();
        complete_operation();
      }
    }

    //--------------------------------------------------------------------------
    void MustEpochOp::notify_subop_commit(Operation *op)
    //--------------------------------------------------------------------------
    {
      bool need_commit;
      {
        AutoLock o_lock(op_lock);
#ifdef DEBUG_HIGH_LEVEL
        assert(remaining_subop_commits > 0);
#endif
        remaining_subop_commits--;
        need_commit = (remaining_subop_commits == 0);
      }
      if (need_commit)
        commit_operation();
    }

    //--------------------------------------------------------------------------
    int MustEpochOp::find_operation_index(Operation *op, GenerationID op_gen)
    //--------------------------------------------------------------------------
    {
      for (unsigned idx = 0; idx < indiv_tasks.size(); idx++)
      {
        if ((indiv_tasks[idx] == op) && 
            (indiv_tasks[idx]->get_generation() == op_gen))
          return idx;
      }
      for (unsigned idx = 0; idx < index_tasks.size(); idx++)
      {
        if ((index_tasks[idx] == op) &&
            (index_tasks[idx]->get_generation() == op_gen))
          return (idx+indiv_tasks.size());
      }
      return -1;
    }

    //--------------------------------------------------------------------------
    TaskOp* MustEpochOp::find_task_by_index(int index)
    //--------------------------------------------------------------------------
    {
      if (index < indiv_tasks.size())
        return indiv_tasks[index];
      index -= indiv_tasks.size();
      if (index < index_tasks.size())
        return index_tasks[index];
      assert(false);
      return NULL;
    }

    /////////////////////////////////////////////////////////////
    // Must Epoch Triggerer 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    MustEpochTriggerer::MustEpochTriggerer(MustEpochOp *own)
      : owner(own)
    //--------------------------------------------------------------------------
    {
      trigger_lock = Reservation::create_reservation();
    }

    //--------------------------------------------------------------------------
    MustEpochTriggerer::MustEpochTriggerer(const MustEpochTriggerer &rhs)
      : owner(rhs.owner)
    //--------------------------------------------------------------------------
    {
      // should never be called
      assert(false);
    }

    //--------------------------------------------------------------------------
    MustEpochTriggerer::~MustEpochTriggerer(void)
    //--------------------------------------------------------------------------
    {
      trigger_lock.destroy_reservation();
      trigger_lock = Reservation::NO_RESERVATION;
    }

    //--------------------------------------------------------------------------
    MustEpochTriggerer& MustEpochTriggerer::operator=(
                                                  const MustEpochTriggerer &rhs)
    //--------------------------------------------------------------------------
    {
      // should never be called
      assert(false);
      return *this;
    }

    //--------------------------------------------------------------------------
    bool MustEpochTriggerer::trigger_tasks(
                                const std::vector<IndividualTask*> &indiv_tasks,
                                std::vector<bool> &indiv_triggered,
                                const std::vector<IndexTask*> &index_tasks,
                                std::vector<bool> &index_triggered)
    //--------------------------------------------------------------------------
    {
      std::deque<IndividualTask*> needed_indiv;
      std::deque<IndexTask*> needed_index;
      std::set<Event> wait_events;
      // Count how many triggerers we are attempting
      for (unsigned idx = 0; idx < indiv_triggered.size(); idx++)
        if (!indiv_triggered[idx])
          needed_indiv.push_back(indiv_tasks[idx]);
      for (unsigned idx = 0; idx < index_triggered.size(); idx++)
        if (!index_triggered[idx])
          needed_index.push_back(index_tasks[idx]);
      // Now do the launches
      if (!needed_indiv.empty())
      {
        Processor util_proc = owner->runtime->find_utility_group();
        MustEpochIndivArgs args;
        args.hlr_id = HLR_MUST_INDIV_ID;
        args.triggerer = this;
        for (unsigned idx = 0; idx < needed_indiv.size(); idx++)
        {
          args.task = needed_indiv[idx];
          Event wait = util_proc.spawn(HLR_TASK_ID, &args, sizeof(args));
          if (wait.exists())
            wait_events.insert(wait);
        }
      }
      if (!needed_index.empty())
      {
        Processor util_proc = owner->runtime->find_utility_group();
        MustEpochIndexArgs args;
        args.hlr_id = HLR_MUST_INDEX_ID;
        args.triggerer = this;
        for (unsigned idx = 0; idx < needed_index.size(); idx++)
        {
          args.task = needed_index[idx];
          Event wait = util_proc.spawn(HLR_TASK_ID, &args, sizeof(args));
          if (wait.exists())
            wait_events.insert(wait);
        }
      }

      // Wait for all of the launches to be done
      // We can safely block to free up the utility processor
      if (!wait_events.empty())
      {
        Event trigger_event = Event::merge_events(wait_events);
        trigger_event.wait(false/*block*/);
      }
      
      // Now see if any failed
      // Otherwise mark which ones succeeded
      if (!failed_individual_tasks.empty())
      {
        for (unsigned idx = 0; idx < indiv_tasks.size(); idx++)
        {
          if (indiv_triggered[idx])
            continue;
          if (failed_individual_tasks.find(indiv_tasks[idx]) ==
              failed_individual_tasks.end())
            indiv_triggered[idx] = true;
        }
      }
      if (!failed_index_tasks.empty())
      {
        for (unsigned idx = 0; idx < index_tasks.size(); idx++)
        {
          if (index_triggered[idx])
            continue;
          if (failed_index_tasks.find(index_tasks[idx]) ==
              failed_index_tasks.end())
            index_triggered[idx] = true;
        }
      }
      return (failed_individual_tasks.empty() && failed_index_tasks.empty());
    }

    //--------------------------------------------------------------------------
    void MustEpochTriggerer::trigger_individual(IndividualTask *task)
    //--------------------------------------------------------------------------
    {
      if (!task->trigger_execution())
      {
        AutoLock t_lock(trigger_lock);
        failed_individual_tasks.insert(task);
      }
    }

    //--------------------------------------------------------------------------
    void MustEpochTriggerer::trigger_index(IndexTask *task)
    //--------------------------------------------------------------------------
    {
      if (!task->trigger_execution())
      {
        AutoLock t_lock(trigger_lock);
        failed_index_tasks.insert(task);
      }
    }

    //--------------------------------------------------------------------------
    /*static*/ void MustEpochTriggerer::handle_individual(const void *args)
    //--------------------------------------------------------------------------
    {
      const MustEpochIndivArgs *indiv_args = (const MustEpochIndivArgs*)args;
      indiv_args->triggerer->trigger_individual(indiv_args->task);
    }

    //--------------------------------------------------------------------------
    /*static*/ void MustEpochTriggerer::handle_index(const void *args)
    //--------------------------------------------------------------------------
    {
      const MustEpochIndexArgs *index_args = (const MustEpochIndexArgs*)args;
      index_args->triggerer->trigger_index(index_args->task);
    }

    /////////////////////////////////////////////////////////////
    // Must Epoch Mapper 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    MustEpochMapper::MustEpochMapper(MustEpochOp *own)
      : owner(own), success(true)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    MustEpochMapper::MustEpochMapper(const MustEpochMapper &rhs)
      : owner(rhs.owner)
    //--------------------------------------------------------------------------
    {
      // should never be called
      assert(false);
    }

    //--------------------------------------------------------------------------
    MustEpochMapper::~MustEpochMapper(void)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    MustEpochMapper& MustEpochMapper::operator=(const MustEpochMapper &rhs)
    //--------------------------------------------------------------------------
    {
      // should never be called
      assert(false);
      return *this;
    }

    //--------------------------------------------------------------------------
    bool MustEpochMapper::map_tasks(const std::set<SingleTask*> &single_tasks)
    //--------------------------------------------------------------------------
    {
      std::set<Event> wait_events;   
      MustEpochMapArgs args;
      args.hlr_id = HLR_MUST_MAP_ID;
      args.mapper = this;
      Processor util_proc = owner->runtime->find_utility_group();
      for (std::set<SingleTask*>::const_iterator it = single_tasks.begin();
            it != single_tasks.end(); it++)
      {
        args.task = *it;
        Event wait = util_proc.spawn(HLR_TASK_ID, &args, sizeof(args));
        if (wait.exists())
          wait_events.insert(wait);
      }
      
      if (!wait_events.empty())
      {
        Event mapped_event = Event::merge_events(wait_events);
        mapped_event.wait(false/*block*/);
      }

      // If we failed to map then unmap all the tasks 
      if (!success)
      {
        for (std::set<SingleTask*>::const_iterator it = single_tasks.begin();
              it != single_tasks.end(); it++)
        {
          (*it)->unmap_all_regions();
        }
      }
      return success;
    }

    //--------------------------------------------------------------------------
    void MustEpochMapper::map_task(SingleTask *task)
    //--------------------------------------------------------------------------
    {
      // Note we don't need to hold a lock here because this is
      // a monotonic change.  Once it fails for anyone then it
      // fails for everyone.
      if (!task->perform_mapping(true/*mapper invoked already*/))
        success = false;
    }

    //--------------------------------------------------------------------------
    /*static*/ void MustEpochMapper::handle_map_task(const void *args)
    //--------------------------------------------------------------------------
    {
      const MustEpochMapArgs *map_args = (const MustEpochMapArgs*)args;
      map_args->mapper->map_task(map_args->task);
    }

    /////////////////////////////////////////////////////////////
    // Must Epoch Distributor 
    /////////////////////////////////////////////////////////////

    //--------------------------------------------------------------------------
    MustEpochDistributor::MustEpochDistributor(MustEpochOp *own)
      : owner(own)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    MustEpochDistributor::MustEpochDistributor(const MustEpochDistributor &rhs)
      : owner(rhs.owner)
    //--------------------------------------------------------------------------
    {
      // should never be called
      assert(false);
    }

    //--------------------------------------------------------------------------
    MustEpochDistributor::~MustEpochDistributor(void)
    //--------------------------------------------------------------------------
    {
    }

    //--------------------------------------------------------------------------
    MustEpochDistributor& MustEpochDistributor::operator=(
                                                const MustEpochDistributor &rhs)
    //--------------------------------------------------------------------------
    {
      // should never be called
      assert(false);
      return *this;
    }

    //--------------------------------------------------------------------------
    void MustEpochDistributor::distribute_tasks(Runtime *runtime,
                                const std::vector<IndividualTask*> &indiv_tasks,
                                const std::set<SliceTask*> &slice_tasks)
    //--------------------------------------------------------------------------
    {
      MustEpochDistributorArgs dist_args;
      dist_args.hlr_id = HLR_MUST_DIST_ID;
      MustEpochLauncherArgs launch_args;
      launch_args.hlr_id = HLR_MUST_LAUNCH_ID;
      std::set<Event> wait_events;
      Processor util_proc = runtime->find_utility_group();
      for (std::vector<IndividualTask*>::const_iterator it = 
            indiv_tasks.begin(); it != indiv_tasks.end(); it++)
      {
        if (!runtime->is_local((*it)->target_proc))
        {
          dist_args.task = *it;
          Event wait = util_proc.spawn(HLR_TASK_ID, 
                                       &dist_args, sizeof(dist_args));
          if (wait.exists())
            wait_events.insert(wait);
        }
        else
        {
          launch_args.task = *it;
          Event wait = util_proc.spawn(HLR_TASK_ID,
                                       &launch_args, sizeof(launch_args));
          if (wait.exists())
            wait_events.insert(wait);
        }
      }
      for (std::set<SliceTask*>::const_iterator it = 
            slice_tasks.begin(); it != slice_tasks.end(); it++)
      {
        if (!runtime->is_local((*it)->target_proc))
        {
          dist_args.task = *it;
          Event wait = util_proc.spawn(HLR_TASK_ID,
                                       &dist_args, sizeof(dist_args));
          if (wait.exists())
            wait_events.insert(wait);
        }
        else
        {
          launch_args.task = *it;
          Event wait = util_proc.spawn(HLR_TASK_ID,
                                       &launch_args, sizeof(launch_args));
          if (wait.exists())
            wait_events.insert(wait);
        }
      }
      if (!wait_events.empty())
      {
        Event dist_event = Event::merge_events(wait_events);
        dist_event.wait(false/*block*/);
      }
    }

    //--------------------------------------------------------------------------
    /*static*/ void MustEpochDistributor::handle_distribute_task(
                                                               const void *args)
    //--------------------------------------------------------------------------
    {
      const MustEpochDistributorArgs *dist_args = 
        (const MustEpochDistributorArgs*)args;
      dist_args->task->distribute_task();
    }

    //--------------------------------------------------------------------------
    /*static*/ void MustEpochDistributor::handle_launch_task(const void *args)
    //--------------------------------------------------------------------------
    {
      const MustEpochLauncherArgs *launch_args = 
        (const MustEpochLauncherArgs *)args;
      launch_args->task->launch_task();
    }

  }; // namespace LegionRuntime
}; // namespace HighLevel

// EOF


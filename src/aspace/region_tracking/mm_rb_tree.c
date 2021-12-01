#include <aspace/region_tracking/mm_rb_tree.h>
#include <nautilus/nautilus.h>

#ifndef NAUT_CONFIG_DEBUG_ASPACE_REGION_TRACKING
#undef DEBUG_PRINT
#define DEBUG_PRINT(fmt, args...) 
#endif

#define ERROR_RB(fmt, args...) ERROR_PRINT("aspace-rbtree: " fmt, ##args)
#define DEBUG_RB(fmt, args...) DEBUG_PRINT("aspace-rbtree: " fmt, ##args)
#define INFO_RB(fmt, args...)   INFO_PRINT("aspace-rbtree: " fmt, ##args)

#define MALLOC_RB(n) \
({ \
    CARAT_PROFILE_INCR(CARAT_DO_PROFILE, num_rb_mallocs); \
    CARAT_PROFILE_INIT_TIMING_VAR(0); \
    CARAT_PROFILE_START_TIMING(CARAT_DO_PROFILE, 0); \
    void *__p = malloc(n); \
    if (!__p) \
    { \
        ERROR_RB("Malloc failed\n"); \
        panic("Malloc failed\n"); \
    } \
    CARAT_PROFILE_STOP_COMMIT_RESET(CARAT_DO_PROFILE, rb_malloc_time, 0); \
    __p; \
})

#define RB_FREE(__ptr) \
({ \
    CARAT_PROFILE_INCR(CARAT_DO_PROFILE, num_rb_frees); \
    CARAT_PROFILE_INIT_TIMING_VAR(0); \
    CARAT_PROFILE_START_TIMING(CARAT_DO_PROFILE, 0); \
    free(__ptr); \
    CARAT_PROFILE_STOP_COMMIT_RESET(CARAT_DO_PROFILE, rb_free_time, 0); \
})



#define NUM2COLOR(n) (((n) == BLACK) ? 'B' : 'R')
#define NODE_STR_LEN 128
#define NODE_STR_DETAIL_LEN (NODE_STR_LEN * 4)

#define REGION_FORMAT "(VA=0x%p to PA=0x%p, len=%lx, prot=%lx)"
#define REGION(r) (r)->va_start, (r)->pa_start, (r)->len_bytes, (r)->protect.flags

// HACK for carat integration
#include <aspace/carat.h>

/*
         |                    |   
         x                    y
       /  \                 /  \
      a    y      ===>     x    c
         /  \            /  \  
        b    c          a    b
*/
void left_rotate(mm_rb_tree_t * tree, mm_rb_node_t *x) {
    mm_rb_node_t * y = x->right;
    
    // transfer b from y to x
    x->right = y->left;
    if (y->left != tree->NIL) {
        y->left->parent = x;
    }

    // link y to x's parent
    y->parent = x->parent;
    if (x->parent == tree->NIL) {
        tree->root = y;
    } else {
        if(x == x->parent->left) {
            x->parent->left = y;
        } else {
            x->parent->right = y;
        }
    }

    // link x and y as left child and parent 
    y->left = x;
    x->parent = y;

}

/*
         |                    |
         x                    y
       /  \                 /  \
      y    a      ===>     b    x
    /  \                      /  \  
   b    c                    c    a
*/
void right_rotate(mm_rb_tree_t * tree, mm_rb_node_t *x) {
    mm_rb_node_t * y = x->left;
    
    // transfer c from y to x
    x->left = y->right;
    if (y->right != tree->NIL) {
        y->right->parent = x;
    }

    // link y to x's parent
    y->parent = x->parent;
    if (x->parent == tree->NIL) {
        tree->root = y;
    } else {
        if(x == x->parent->left) {
            x->parent->left = y;
        } else {
            x->parent->right = y;
        }
    }

    // link x and y as left child and parent 
    y->right = x;
    x->parent = y;
}

void rb_tree_insert_fixup(mm_rb_tree_t * tree, mm_rb_node_t * z){
    mm_rb_node_t * y;
    char buf[NODE_STR_DETAIL_LEN];

    while (z->parent->color == RED) {
        // node2str_detail(tree, z, buf);
        // printf("%s\n", buf);
        if (z->parent == z->parent->parent->left) {
            y = z->parent->parent->right;
            if (y->color == RED) {
                //case 1 pop red node up
                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                y->color = BLACK;
                z = z->parent->parent;
            } else {
                if (z == z->parent->right) {
                    // case 2, converts to case 3
                    z = z->parent;
                    left_rotate(tree, z);
                }
                // case 3 solve the red conflict
                
                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                right_rotate(tree, z->parent->parent);
            
            }
        } else {
            y = z->parent->parent->left;
            if (y->color == RED) {
                //case 1 pop red node up
                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                y->color = BLACK;
                z = z->parent->parent;
            } else {
                if (z == z->parent->left) {
                    // case 2, converts to case 3
                    z = z->parent;
                    right_rotate(tree, z);
                }
                // case 3 solve the red conflict
                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                left_rotate(tree, z->parent->parent);
            }
        }
    }
    tree->root->color = BLACK;
}

int rb_tree_insert(mm_struct_t * self, nk_aspace_region_t * region) {
    mm_rb_tree_t * tree = (mm_rb_tree_t * ) self;

    mm_rb_node_t * curr = tree->root;
    mm_rb_node_t * parent = tree->NIL;

    mm_rb_node_t * wrapper = (mm_rb_node_t *) MALLOC_RB(sizeof(mm_rb_node_t));
    
    wrapper->region = *region;
    
    while (curr != tree->NIL) {
        int comp = (*tree->compf)(wrapper, curr);
        parent = curr;
        if (comp < 0) {
            curr = curr->left;
        } else {
            curr = curr->right;
        }
        
    }
    wrapper->parent = parent;
    
    if (parent == tree->NIL) {
        tree->root = wrapper;
    } else {
        int comp = (*tree->compf)(wrapper, parent);
        if (comp < 0) {
            parent->left = wrapper;
        } else {
            parent->right = wrapper;
        }
    }

    wrapper->left = tree->NIL;
    wrapper->right = tree->NIL;
    wrapper->color = RED;
    rb_tree_insert_fixup(tree, wrapper);

    tree->super.size = tree->super.size + 1;

    return 0;
}

mm_rb_node_t * rb_tree_search(mm_rb_tree_t * tree, mm_rb_node_t * node) {
    // mm_rb_tree_t * tree = (mm_rb_tree_t * ) self;

    mm_rb_node_t * curr = tree->root;
    
    while (curr != tree->NIL) {
        int comp = (*tree->compf)(node, curr);
        if (comp < 0) {
            curr = curr->left;
        } else if (comp > 0) {
            curr = curr->right;
        } else {
            return curr;
        }        
    }

    return tree->NIL;
}

/*
    return tree->NIL if tree is empty
*/
mm_rb_node_t * rb_tree_minimum(mm_rb_tree_t * tree, mm_rb_node_t * node) {
    mm_rb_node_t * curr = node;
    mm_rb_node_t * parent = curr;
    while (curr != tree->NIL) {
        parent = curr;
        curr = curr->left;
    }
    
    return parent;
}

/*
    return tree->NIL if tree is empty
*/
mm_rb_node_t * rb_tree_maximum(mm_rb_tree_t * tree, mm_rb_node_t * node) {
    mm_rb_node_t * curr = tree->root;
    mm_rb_node_t * parent = tree->NIL;
    while (curr != tree->NIL) {
        parent = curr;
        curr = curr->right;
    }
    
    return parent;
}

void mm_rb_tree_transplant(mm_rb_tree_t * tree, mm_rb_node_t * u, mm_rb_node_t * v) {
    if (u->parent == tree->NIL) {
        tree->root = v;
    } else if (u == u->parent->left) {
        u->parent->left = v;
    } else {
        u->parent->right = v;
    }

    v->parent = u->parent;
}

void rb_tree_delete_fixup(mm_rb_tree_t * tree, mm_rb_node_t * x){
    mm_rb_node_t * w;
    while (x != tree->root && x->color == BLACK) {
        // char buf[NODE_STR_DETAIL_LEN];
        // node2str_detail(tree, x, buf);
        // printf("extra blackness at %s\n", buf);
        if (x == x->parent->left){
            w = x->parent->right;

            // node2str_detail(tree, w, buf);
            // printf("w = %s\n", buf);

            if (w->color == RED) {
                /*
                    case 1: brother red
                    converts to cases 2, 3, 4
                      (p, B)                    (w, B)
                       /  \                      /  \
                  (x, B)  (w, R)    ==>     (p, R)  (b, B)
                         /    \             /   \
                       (a, B) (b, B)    (x, B) (a, B) 
                */
                w->color = BLACK;
                x->parent->color = RED;
                left_rotate(tree, x->parent);
                w = x->parent->right;
            }
            // now we have brother black. 
        
            if (w->left->color == BLACK && w->right->color == BLACK) {
                /*
                    case 2: w black and both children of w black
                    move the extra blackness up.
                    Note that if we enter case 2 from case 1, x->parent->color == RED, so loop terminates there 
                        (p, U)                    (p, U)  new x
                        /  \                      /  \
                    (x, B)  (w, B)    ==>     (x, B)  (w, R)
                            /    \                    /   \
                        (a, B) (b, B)             (a, B) (b, B) 
                */
                w->color = RED;
                x = x->parent;
            } else {
                if (w->left->color == RED && w->right->color == BLACK) {
                    /*
                        case 3: w black and left child RED but right child BLACK
                        we convet to case 4
                            (p, U)                    (p, U)  
                            /  \                      /  \
                        (x, B)  (w, B)    ==>     (x, B)  (a, B) new w
                                /    \                       \
                            (a, R) (b, B)                  (w, R)
                                                               \
                                                             (b, B) 
                    */
                    w->left->color = BLACK;
                    w->color = RED;
                    right_rotate(tree, w);
                    w = x->parent->right;
                }
                /*
                    case 4: w black and right child RED
                    left rotate the tree and resolve the conflict
                        (p, U)                          (w, U)
                        /  \                            /    \
                    (x, B)  (a, B) new w    ==>     (p, B)  (d, B)
                                \                     /        
                                (d, R)            (x, B)       
                */        
                w->color = x->parent->color;
                x->parent->color = BLACK;
                w->right->color = BLACK;
                left_rotate(tree, x->parent);
                x = tree->root;
            }

        } else {
            /*
                symmetric to the cases elaborated above,
                change left to right, right to left, 
                and left_rotate to righ_rotate and right_rotate to left_rotate
            */

            w = x->parent->left;

            // node2str_detail(tree, w, buf);
            // printf("w = %s\n", buf);

            if (w->color == RED) {
                w->color = BLACK;
                x->parent->color = RED;
                right_rotate(tree, x->parent);
                w = x->parent->left;
            }

            if (w->left->color == BLACK && w->right->color == BLACK) {
                w->color = RED;
                x = x->parent;
            } else {
                if (w->left->color == BLACK) {
                    w->right->color = BLACK;
                    w->color = RED;
                    left_rotate(tree, w);
                    w = x->parent->left;
                }
                w->color = x->parent->color;
                x->parent->color = BLACK;
                w->left->color = BLACK;
                right_rotate(tree, x->parent);
                x = tree->root;
            }
        }

    }
    x->color = BLACK;
}

void rb_tree_delete_node(mm_rb_tree_t * tree, mm_rb_node_t * z) {
    if (z == tree->NIL || z == NULL) {
        return;
    }

    mm_rb_node_t * y, * x;
    enum rb_tree_node_color original_color;
    if (z->left == tree->NIL || z->right == tree->NIL) {
        original_color = z->color;
        if (z->left == tree->NIL) {
            x = z->right;
            mm_rb_tree_transplant(tree, z, z->right);
        }else {
            x = z->left;
            mm_rb_tree_transplant(tree, z, z->left);
        }
        
    } else {
        /*
            In this case, z, the node we try to move, has two non trivial children.
            y would be the node to replace z, and x would be the node to replace y. 
        */
        y = rb_tree_minimum(tree, z->right);
        original_color = y->color;
        
        /*
            Because we are using the minimum within the subtree under z->right as y, 
            y-left must be tree->NIL.
        */

        x = y->right;
        if (y->parent == z) {
            x->parent = y;
        } else {
            mm_rb_tree_transplant(tree, y, y->right);
            y->right = z->right;
            y->right->parent = y;
        }

        mm_rb_tree_transplant(tree, z, y);
        y->left = z->left;
        y->left->parent = y;
        y->color = z->color;
    }

    // If y is originally a black one, we need to call the fixup function to fix the extra blackness. 
    // printf("try to fixup\n");
    if (original_color == BLACK) rb_tree_delete_fixup(tree, x);
    RB_FREE(z);

    tree->super.size = tree->super.size - 1;
}

int node2str(mm_rb_tree_t * tree, mm_rb_node_t * node, char * str) {
    if (node == tree->NIL){
        sprintf(str, "NIL");
    } else {
        sprintf(str, "((VA = 0x%016lx to PA = 0x%016lx, len = %lx, prot=%lx) %c)", 
            node->region.va_start,
            node->region.pa_start,
            node->region.len_bytes,
            node->region.protect.flags,
            NUM2COLOR(node->color)
        );
    }

    int len = strlen(str);

    if (len > NODE_STR_LEN) {
        ERROR_RB("running out of allocated space when printing node!\n");
        return -1;
    }
    return strlen(str);
}

long max_depth_helper(mm_rb_tree_t * tree, mm_rb_node_t * curr) {
    if (curr == tree->NIL) return 0;
    long left_depth = max_depth_helper(tree, curr->left);
    long right_depth = max_depth_helper(tree, curr->right);

    return 1 + (left_depth > right_depth ? left_depth : right_depth);
}

long max_depth(mm_rb_tree_t * tree) {
    return max_depth_helper(tree, tree->root);
}

void rb_tree_inorder_helper(mm_rb_tree_t * tree, mm_rb_node_t * curr) {
    if (curr != tree->NIL) {
        rb_tree_inorder_helper(tree, curr->left);
        DEBUG_RB("(VA = 0x%016lx to PA = 0x%016lx, len = %lx, prot=%lx)\n", 
            curr->region.va_start,
            curr->region.pa_start,
            curr->region.len_bytes,
            curr->region.protect.flags
        );
        rb_tree_inorder_helper(tree, curr->right);
    }
}


void rb_tree_inorder(mm_struct_t * self){
    mm_rb_tree_t * tree = (mm_rb_tree_t * ) self;
    DEBUG_RB("size = %d (will not print if depth >= MAX_SIZE_INORDER_PRINT = %d)\n",
        tree->super.size, MAX_SIZE_INORDER_PRINT
    );

    if (tree->super.size < MAX_SIZE_INORDER_PRINT) {
        rb_tree_inorder_helper(tree, tree->root);
    }

    if (RB_TREE_CHECK_AFTER_TRAVERSAL){
        rb_tree_check(self);
        DEBUG_RB("rb tree at %p passed rb property check!\n", tree);
    }

}

void rb_tree_level_order(mm_struct_t * self) {

    mm_rb_tree_t * tree = (mm_rb_tree_t *) self;
    DEBUG_RB("Displaying the tree at %p with levelorder BFS\n", tree );

    long depth = max_depth(tree);
    DEBUG_RB("depth = %d (will not print if depth >= MAX_DEPTH_LEVEL_ORDER_PRINT = %d)\n",
        depth, MAX_DEPTH_LEVEL_ORDER_PRINT
    );

    if (depth < MAX_DEPTH_LEVEL_ORDER_PRINT){
        char buf[NODE_STR_LEN * (1 << depth)];
        char * free_space_ptr = buf;

        size_t queue_size = (1 << depth) - 1;
    
        int head = 0, tail = 0, next_power = 1, curlevel = 0;
        
        mm_rb_node_t ** q = (mm_rb_node_t ** ) MALLOC_RB(queue_size * sizeof(mm_rb_node_t *));
        mm_rb_node_t * curr = tree->root;
        
        if (curr == tree->NIL) {
            DEBUG_RB("empty tree!\n");
            return;
        }

        q[tail++] = curr;
        
        while (head != queue_size) {
            curr = q[head];
            
            int len = node2str(tree, curr, free_space_ptr);
            if (len < 0 ) {
                ERROR_RB("level order fail because of not enough allocated space!\n");
                return;
            }
            free_space_ptr += len;
            *free_space_ptr++ = ' ';


            if (tail < queue_size) {
                if (curr != tree->NIL){
                    q[tail++] = curr->left;
                    q[tail++] = curr->right;
                } else {
                    q[tail++] = tree->NIL;
                    q[tail++] = tree->NIL;
                }
            }
            
            head++;
            if (head == next_power) {
                *free_space_ptr++ = '\0';
                DEBUG_RB("%s\n", buf);
                free_space_ptr = buf;
                curlevel++;
                next_power += (1 << curlevel);

            }
        }
    }

    if (RB_TREE_CHECK_AFTER_TRAVERSAL){
        rb_tree_check(self);
        DEBUG_RB("rb tree at %p passed rb property check!\n", tree);
    }

}

int rb_tree_check_helper(mm_rb_tree_t * tree, mm_rb_node_t * node) {
    // printf("checking");
    // disp_node(tree, node);
    // printf("\n");
    char buf[NODE_STR_LEN];
    node2str(tree, node, buf);

    int color_test = node->color == BLACK || node->color == RED;

    if (!color_test) {
        panic("%s color test fail with color=%d\n", buf, node->color);
        // assert(color_test);
    }
    

    if (node->color == RED) {
        int red_color_test = (node->left->color == BLACK && node->left->color == BLACK);
        
        if (!red_color_test) {
            panic("%s red color test fail with leftchild->color=%c rightchild->color=%c\n",
                    buf,
                    NUM2COLOR(node->left->color),
                    NUM2COLOR(node->right->color)
            );
        }
        
    }


    if (node == tree->NIL) return 0;
    int left_bh = rb_tree_check_helper(tree, node->left);
    int right_bh = rb_tree_check_helper(tree, node->right);


    int bh_test = (left_bh == right_bh);

    if (!bh_test) {
        panic("%s BH test fail with left_bh=%d right_bh=%d\n", 
                buf, left_bh, right_bh
        );
    }

    return left_bh + (node->color == BLACK);
}

void BST_order_check_helper(mm_rb_tree_t * tree, mm_rb_node_t * curr, mm_rb_node_t * prev) {
    if (curr != tree->NIL) {
        BST_order_check_helper(tree, curr->left, prev);

        if (prev != tree->NIL) {
            int comp = (*tree->compf)(prev, curr);
            if (comp > 0) {
                char buf_prev[NODE_STR_LEN];
                char buf_curr[NODE_STR_LEN];
                node2str(tree, prev, buf_prev);
                node2str(tree, curr, buf_curr);
                panic("%s and %s have order arranged wrong!\n", buf_prev, buf_curr);
            }
        }
        prev = curr;
        BST_order_check_helper(tree, curr->right, prev);
    }
}   
/* 
    test BST property
*/
void BST_order_check(mm_rb_tree_t * tree) {
    // printf("Checking the tree at %p with inorder (left, root, right)"
    //         "expect ascending order\n",
    //         tree
    //     );
    BST_order_check_helper(tree, tree->root, tree->NIL);
}

int rb_tree_check(mm_struct_t * self) {
    mm_rb_tree_t * tree = (mm_rb_tree_t *) self;

    BST_order_check(tree);

    int root_black_test = (tree->root->color == BLACK);
    if (!root_black_test) {
        panic("ERROR: root color=%d\n", tree->root->color);
    }
    
    int NIL_black_test = (tree->NIL->color == BLACK);
    if (!NIL_black_test) {
        panic("ERROR: NIL color=%d\n", tree->NIL->color);
    }
    rb_tree_check_helper(tree, tree->root);

    return 0;
}


/*
    dir = 1 select min
    dir = -1 select max
*/
mm_rb_node_t * new_bound(
    mm_rb_tree_t * tree, 
    mm_rb_node_t * prev_upper, 
    mm_rb_node_t * curr_upper,
    int dir
) {
    if (prev_upper == tree->NIL) {
        return curr_upper;
    }

    int comp = (*tree->compf) (prev_upper, curr_upper);
    if (comp * dir <= 0) {
        return prev_upper;
    } else {
        return curr_upper;
    }
}


/*
    find lowest upper bound for key among the elements in tree
    return tree->NIL if key is larger than all elements in the tree
    AKA, no upper bound
    Note: this helper function differs from next_smallest because this is NOT strict upper bound
*/
mm_rb_node_t * rb_tree_LUB(mm_rb_tree_t * tree, mm_rb_node_t * node) {
    mm_rb_node_t * curr = tree->root;
    mm_rb_node_t * upper_bound = tree->NIL;


    while (curr != tree->NIL) {
        int comp = (*tree->compf) (node, curr);
        if (comp == 0){
            upper_bound = curr;
            break;
        } else if (comp < 0) {
            upper_bound = new_bound(tree, upper_bound, curr, 1);
            curr = curr->left;
        } else {
            curr = curr->right;
        }
    }

    return upper_bound;
}
/*
    find greatest lower bound for key among the elements in tree
    return tree->NIL if key is samller than all elements in the tree
    AKA, no lower bound
    Note: this helper function differs from prev_largest because this is NOT strict lower bound
*/
mm_rb_node_t * rb_tree_GLB(mm_rb_tree_t * tree, mm_rb_node_t * node) {
    mm_rb_node_t * curr = tree->root;
    mm_rb_node_t * lower_bound = tree->NIL;

    while (curr != tree->NIL) {
        int comp = (*tree->compf) (node, curr);
        if (comp == 0){
            lower_bound = curr;
            break;
        } else if (comp > 0) {
            lower_bound = new_bound(tree, lower_bound, curr, -1);
            curr = curr->right;
        } else {

            curr = curr->left;
        }
    }

    return lower_bound;
}


/**
 *  return NIL when node is the largest
 * */
mm_rb_node_t * rb_tree_next_smallest(mm_rb_tree_t * tree, mm_rb_node_t * node) {
    mm_rb_node_t * right_min = rb_tree_minimum(tree, node->right);
    if (right_min != tree->NIL) return right_min;
    
    /**
     *  If @node is a leaf,
     *      find the first upstream which is the left child of its parent 
     * */

    while (node != node->parent->left && node != tree->root) {
        node = node->parent;
    }   
    return node->parent;
}

mm_rb_node_t * rb_tree_prev_largest(mm_rb_tree_t * tree, mm_rb_node_t * node) {
    mm_rb_node_t * left_max = rb_tree_maximum(tree, node->left);
    if (left_max != tree->NIL) return left_max;
    
    /**
     *  If @node is a leaf,
     *      find the first upstream which is the right child of its parent 
     * */

    while (node != node->parent->right && node != tree->root) {
        node = node->parent;
    }   
    return node->parent;
}

nk_aspace_region_t * rb_tree_next_smallest_wrap ( mm_struct_t * self, nk_aspace_region_t * cur_region) {
    mm_rb_tree_t * tree = (mm_rb_tree_t *) self;
    
    mm_rb_node_t node;
    node.region = *cur_region;
    
    mm_rb_node_t * target = rb_tree_search(tree, &node);
    
    if (target == tree->NIL) {
        /* no such region */
        DEBUG_RB("input region is not found"REGION_FORMAT"\n", REGION(cur_region));
        return NULL;
    }
    
    mm_rb_node_t * next_smallest = rb_tree_next_smallest(tree, target);
    
    if (next_smallest == tree->NIL) {
        /* the input region is the largest in the tree */
        DEBUG_RB("the input region (" REGION_FORMAT ") is the largest\n", REGION(cur_region));
        return cur_region;
    }

    return &next_smallest->region;
    
}

nk_aspace_region_t * rb_tree_prev_largest_wrap (mm_struct_t * self, nk_aspace_region_t * cur_region){
    mm_rb_tree_t * tree = (mm_rb_tree_t *) self;
    mm_rb_node_t node;
    node.region = *cur_region;
    
    mm_rb_node_t * target = rb_tree_search(tree, &node);
    
    if(target == tree->NIL){
         /* no such region */
        DEBUG_RB("input region is not found"REGION_FORMAT"\n", REGION(cur_region));
        return NULL;
    }

    mm_rb_node_t * prev_largest = rb_tree_prev_largest(tree,target);

    if(prev_largest == tree->NIL){
        /* the input region is the smallest in the tree*/
        DEBUG_RB("the input region (" REGION_FORMAT ") is the smallest\n", REGION(cur_region));
        return cur_region;
    }

    return &prev_largest->region;
}


/*
nk_aspace_region_t * mm_rb_tree_check_overlap(mm_struct_t * self, nk_aspace_region_t * region) {
    mm_rb_tree_t * tree = (mm_rb_tree_t *) self;

    mm_rb_node_t wrapper;
    wrapper.region = * region;

    mm_rb_node_t * GLB = rb_tree_GLB(tree, &wrapper);
    mm_rb_node_t * LUB = rb_tree_LUB(tree, &wrapper);

    if (LUB == NULL ) {
        panic("get lowest upperbound fails!\n");
        return NULL;
    }

    if (LUB == NULL ) {
        panic("get greatest lower bound fails!\n");
        return NULL;
    }

    if (GLB != tree->NIL) {
        nk_aspace_region_t * curr_region_ptr = &GLB->region;
        if (overlap_helper(curr_region_ptr, region)) {
            return curr_region_ptr;
        }
    } 

    if (LUB != tree->NIL) {
        nk_aspace_region_t * curr_region_ptr = &LUB->region;
        if (overlap_helper(curr_region_ptr, region)) {
            return curr_region_ptr;
        }
    }

    return NULL;
}*/

nk_aspace_region_t * mm_rb_tree_check_overlap(mm_struct_t * self, nk_aspace_region_t * region){
    mm_rb_tree_t * tree = (mm_rb_tree_t *) self;
    mm_rb_node_t * curr_node = tree->root;
    mm_rb_node_t wrapper;
    wrapper.region = *region;

    while (curr_node != tree->NIL) {
        int comp = (*tree->compf)(&wrapper, curr_node);

        if(comp == 0 || overlap_helper(region,&curr_node->region)){ 
            return &curr_node->region;
        } else {
            if(comp > 0){
                curr_node = curr_node->right;
            } else{
                curr_node = curr_node->left;
            }
        }
    }

    return NULL;
}

nk_aspace_region_t * rb_tree_find_reg_at_addr(mm_struct_t * self, addr_t address) {
    mm_rb_tree_t * tree = (mm_rb_tree_t *) self;

    mm_rb_node_t node;
    node.region.va_start = (void *) address;
    node.region.len_bytes = 0;

    mm_rb_node_t * GLB = rb_tree_GLB(tree, &node);

    if (GLB == tree->NIL) return NULL;
    // DEBUG("(VA = 0x%016lx to PA = 0x%016lx, len = %lx, prot=%lx)\n", 
    //         GLB->region.va_start,
    //         GLB->region.pa_start,
    //         GLB->region.len_bytes,
    //         GLB->region.protect.flags
    //     );
    nk_aspace_region_t * curr_region_ptr = &GLB->region;
    if (overlap_helper(curr_region_ptr, &node.region)) {
        return curr_region_ptr;
    }

    return NULL;
    
}

nk_aspace_region_t * rb_tree_update_region (
    mm_struct_t * self, 
    nk_aspace_region_t * cur_region, 
    nk_aspace_region_t * new_region, 
    uint8_t eq_flag
) {
    mm_rb_tree_t * tree = (mm_rb_tree_t *) self;

    if (!(eq_flag & VA_CHECK)) {
        ERROR_RB("rb tree expect to update regions with VA as the same!\n");
        return NULL;
    }

    if (!(region_equal(cur_region, new_region, eq_flag))){
        ERROR_RB("rb tree expect to have the input region equal in terms of eq_flag!\n");
        return NULL;
    }

    mm_rb_node_t node;
    node.region = *cur_region;

    mm_rb_node_t * target_node = rb_tree_search(tree, &node);
    
    // region not found
    if (target_node == tree->NIL) return NULL;
    
    // criterion not met
    nk_aspace_region_t * target_region = &target_node->region;
    
    int eq = region_equal(target_region, cur_region, all_eq_flag);
    if (!eq) return NULL;

    region_update(target_region, new_region, eq_flag);

    // if (!(eq_flag & PA_CHECK)) {
    //     target_region->pa_start = new_region->pa_start;
    // }

    // if (!(eq_flag & LEN_CHECK)) {
    //     if (new_region->len_bytes > target_region->len_bytes) {
    //         /**
    //          *  We have to take care here if new region has longer length
    //          *  Expanding current region might lead to overlapping
    //          * */
    //         mm_rb_node_t * next_node = rb_tree_next_smallest(tree, target_node);
    //         int new_region_overlap = overlap_helper(new_region, &next_node->region);
            
    //         if (new_region_overlap){
    //             /**
    //              *  If new region overlaps with existed region, the update the is not valid, and we have to undo the deletion
    //              * */
    //             return NULL;
    //         } 
    //     } 

    //     target_region->len_bytes = new_region->len_bytes;  
    // }

    // if (!(eq_flag & PROTECT_CHECK)) {
    //     target_region->protect = new_region->protect;
    // }

    return target_region;
}

int rb_tree_remove(mm_struct_t * self, nk_aspace_region_t * region, uint8_t check_flags) {
    mm_rb_tree_t * tree = (mm_rb_tree_t *) self;

    if (!(check_flags & VA_CHECK)) {
        ERROR_RB("rb tree expect to remove regions with VA_check flag set!\n");
        return -1;
    }
    mm_rb_node_t node;
    node.region = *region;
    
    mm_rb_node_t * target = rb_tree_search(tree, &node);
    
    // region not found

    if (target == tree->NIL) return -1;
    // criterion not met
    if (!region_equal(&target->region, region, check_flags)) return -1;

    rb_tree_delete_node(tree,target);

    return 0;
}

nk_aspace_region_t* rb_tree_contains(
    mm_struct_t * self, 
    nk_aspace_region_t * region, 
    uint8_t check_flags
) {
    mm_rb_tree_t * tree = (mm_rb_tree_t *) self;

    if (!(check_flags & VA_CHECK)) {
        ERROR_RB("rb tree expect to search regions with VA_check flag set!\n");
        return NULL;
    }

    mm_rb_node_t node;
    node.region = *region;

    mm_rb_node_t * target = rb_tree_search(tree, &node);
    
    // region not found
    if (target == tree->NIL) return NULL;
    // criterion not met
    if (!region_equal(&target->region, region, check_flags)) return NULL;

    return &target->region;
}

int rb_comp_region(mm_rb_node_t * n1, mm_rb_node_t * n2) {
    if (n1->region.va_start < n2->region.va_start) {
        return -1;
    } else if (n1->region.va_start > n2->region.va_start) {
        return 1;
    } else {
        return 0;
    }
}

int mm_rb_node_destroy(mm_rb_tree_t * tree, mm_rb_node_t * node) {
    if (node != tree->NIL) {
        mm_rb_node_destroy(tree, node->left);
        mm_rb_node_destroy(tree, node->right);

        RB_FREE(node);
    }
    return 0;
}

int mm_rb_tree_destroy(mm_struct_t * self) {
    DEBUG_RB("Try to destroy rb tree at %p\n", self);
    mm_rb_tree_t * tree = (mm_rb_tree_t *) self;

    mm_rb_node_destroy(tree, tree->root);
    RB_FREE(tree->NIL);

    RB_FREE(tree);

    DEBUG_RB("Done: rbtree destroyed!\n");
    return 0;
}

mm_rb_node_t * create_rb_NIL() {
    mm_rb_node_t * nil = (mm_rb_node_t *) MALLOC_RB(sizeof(mm_rb_node_t));

    nil->color = BLACK;
    nil->parent = nil->left = nil->right = NULL;
    nk_aspace_region_t reg_default = {0};

    nil->region = reg_default;
    return nil;
}

mm_struct_t * mm_rb_tree_create() {
    // mm_rb_tree_t * rbtree = (mm_rb_tree_t *) MALLOC_RB(sizeof(mm_rb_tree_t));
    mm_rb_tree_t * rbtree = (mm_rb_tree_t *) malloc(sizeof(mm_rb_tree_t));

    if (!rbtree) {
        ERROR_RB("Failed to allocate rbtree with size of %d\n", sizeof(mm_rb_tree_t));
        panic("Failed to allocate rbtree with size of %d\n", sizeof(mm_rb_tree_t));
    }

    DEBUG_RB("allocate rbtree at %p with size of %d\n",rbtree,  sizeof(mm_rb_tree_t));
    mm_struct_init(&rbtree->super);
    
    rbtree->super.vptr->insert = &rb_tree_insert;
    rbtree->super.vptr->show = &rb_tree_inorder;
    // rbtree->super.vptr->show = &rb_tree_level_order;
    rbtree->super.vptr->check_overlap = &mm_rb_tree_check_overlap;
    rbtree->super.vptr->find_reg_at_addr = &rb_tree_find_reg_at_addr;
    rbtree->super.vptr->update_region = &rb_tree_update_region;
    rbtree->super.vptr->remove = &rb_tree_remove;
    rbtree->super.vptr->contains = &rb_tree_contains;
    rbtree->super.vptr->next_smallest = &rb_tree_next_smallest_wrap;
    rbtree->super.vptr->prev_largest = &rb_tree_prev_largest_wrap;

    rbtree->super.vptr->destroy = &mm_rb_tree_destroy;

    rbtree->NIL = create_rb_NIL();
    rbtree->root = rbtree->NIL;
    rbtree->compf = &rb_comp_region;

    return &rbtree->super;
}

mm_rb_tree_t * mm_rb_tree_create_actual_rb_tree() {
    // mm_rb_tree_t * rbtree = (mm_rb_tree_t *) MALLOC_RB(sizeof(mm_rb_tree_t));
    mm_rb_tree_t * rbtree = (mm_rb_tree_t *) malloc(sizeof(mm_rb_tree_t));

    if (!rbtree) {
        ERROR_RB("Failed to allocate rbtree with size of %d\n", sizeof(mm_rb_tree_t));
        panic("Failed to allocate rbtree with size of %d\n", sizeof(mm_rb_tree_t));
    }

    DEBUG_RB("allocate rbtree at %p with size of %d\n",rbtree,  sizeof(mm_rb_tree_t));
    mm_struct_init(&rbtree->super);
    
    rbtree->super.vptr->insert = &rb_tree_insert;
    rbtree->super.vptr->show = &rb_tree_inorder;
    // rbtree->super.vptr->show = &rb_tree_level_order;
    rbtree->super.vptr->check_overlap = &mm_rb_tree_check_overlap;
    rbtree->super.vptr->find_reg_at_addr = &rb_tree_find_reg_at_addr;
    rbtree->super.vptr->update_region = &rb_tree_update_region;
    rbtree->super.vptr->remove = &rb_tree_remove;
    rbtree->super.vptr->contains = &rb_tree_contains;
    rbtree->super.vptr->next_smallest = &rb_tree_next_smallest_wrap;
    rbtree->super.vptr->prev_largest = &rb_tree_prev_largest_wrap;

    rbtree->super.vptr->destroy = &mm_rb_tree_destroy;

    rbtree->NIL = create_rb_NIL();
    rbtree->root = rbtree->NIL;
    // rbtree->compf = &rb_comp_region;

    return rbtree;
}



/*
 * RB Tree Bologna
 */ 
int rb_comp_alloc_entry(mm_rb_node_t * n1, mm_rb_node_t * n2) {

    /*
     * HACK --- We're going to save as much of the current
     * implementation as possible --- cast each of @n1 and
     * @n2's region field as an allocation_entry
     */ 

    allocation_entry *n1_ae = ((allocation_entry *) &(n1->region));
    allocation_entry *n2_ae = ((allocation_entry *) &(n2->region));

    if (n1_ae->pointer < n2_ae->pointer) {
        return -1;
    } else if (n1_ae->pointer > n2_ae->pointer) {
        return 1;
    } else {
        return 0;
    }
}

int rb_comp_escape(mm_rb_node_t * n1, mm_rb_node_t * n2) {

    /*
     * HACK --- We're going to save as much of the current
     * implementation as possible --- cast each of @n1 and
     * @n2's region field as an void ** (representing an 
     * actual escape)
     */ 

    void **n1_escape = ((void **) &(n1->region));
    void **n2_escape = ((void **) &(n2->region));

    if (n1_escape < n2_escape) {
        return -1;
    } else if (n1_escape > n2_escape) {
        return 1;
    } else {
        return 0;
    }
}


int rb_tree_remove_alloc(mm_struct_t * self, nk_aspace_region_t * region, uint8_t check_flags) {
    
    /*
     * HACK --- Ignore @check_flags
     */ 
    mm_rb_tree_t * tree = (mm_rb_tree_t *) self;

    mm_rb_node_t node;
    
    allocation_entry fake_entry = {
        .pointer = (void *) region
    } ;
   
    node.region = *((nk_aspace_region_t *) &fake_entry);
    
    mm_rb_node_t * target = rb_tree_search(tree, &node);
    if (target == tree->NIL) return -1;
    // --- NOTE --- removed region_equal functionality

    rb_tree_delete_node(tree,target);

    return 0;
}


int rb_tree_remove_escape(mm_struct_t * self, nk_aspace_region_t * region, uint8_t check_flags) {
    
    /*
     * HACK --- Ignore @check_flags
     */ 
    mm_rb_tree_t * tree = (mm_rb_tree_t *) self;

    mm_rb_node_t node;
   
    void **escape = (void **) region;
    node.region = *((nk_aspace_region_t *) &escape); 
    
    mm_rb_node_t * target = rb_tree_search(tree, &node);
    if (target == tree->NIL) return -1;
    // --- NOTE --- removed region_equal functionality

    rb_tree_delete_node(tree,target);

    return 0;
}


nk_aspace_region_t * rb_tree_find_allocation_entry_from_addr(mm_struct_t * self, addr_t address) {
    mm_rb_tree_t * tree = (mm_rb_tree_t *) self;

    mm_rb_node_t node;
    allocation_entry fake_entry = {
        .pointer = (void *) address
    } ;
   
    node.region = *((nk_aspace_region_t *) &fake_entry);

    mm_rb_node_t * GLB = rb_tree_GLB(tree, &node);

    if (GLB == tree->NIL) return NULL;
    
    nk_aspace_region_t * curr_region_ptr = &GLB->region;
    allocation_entry *real_returned_entry = ((allocation_entry *) curr_region_ptr);
    void *returned_entry_pointer = real_returned_entry->pointer;
    uint64_t returned_entry_size = real_returned_entry->size;

    if (true
        && (address >= ((addr_t) returned_entry_pointer))
        && (address < (((uint64_t) returned_entry_pointer) + returned_entry_size))) {
        return curr_region_ptr;
    }

    return NULL;
    
}

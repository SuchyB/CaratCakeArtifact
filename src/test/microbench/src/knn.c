/* 
 * This file is part of the Nautilus AeroKernel developed
 * by the Hobbes and V3VEE Projects with funding from the 
 * United States National  Science Foundation and the Department of Energy.  
 *
 * The V3VEE Project is a joint project between Northwestern University
 * and the University of New Mexico.  The Hobbes Project is a collaboration
 * led by Sandia National Laboratories that includes several national 
 * laboratories and universities. You can find out more at:
 * http://www.v3vee.org  and
 * http://xstack.sandia.gov/hobbes
 *
 * Copyright (c) 2020, Souradip Ghosh <sgh@u.northwestern.edu>
 * Copyright (c) 2020, The V3VEE Project  <http://www.v3vee.org> 
 *                     The Hobbes Project <http://xstack.sandia.gov/hobbes>
 * All rights reserved.
 *
 * Author: Souradip Ghosh <sgh@u.northwestern.edu>
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "LICENSE.txt".
 */

/*
 * Self contained KNN implementation that's compatible with Nautilus. 
 * Used as a microbenchmark for testing the compiler-timing model on fibers
 */ 

#include <nautilus/nautilus.h>
#include <test/fiberbench/knn.h>
#include <nautilus/random.h>
#include <nautilus/libccompat.h>

// Useful macros
#define MIN(a, b) ( ((a) < (b)) ? (a) : (b) )
#define MAX(a, b) ( ((a) > (b)) ? (a) : (b) )
#define ABS(a) ( ((a) < 0) ? (-(a)) : (a) )
#define LEN(arr) ( (sizeof(arr)) / (sizeof(arr[0])) )
#define MALLOC(n) ({void *__p = malloc(n); if (!__p) { panic("Malloc failed\n"); } __p;})
#define SEED() (srand48(rdtsc() % 128))

// Setup
struct KNNPoint *KNN_build_point(uint32_t dims)
{
	// Allocate
	struct KNNPoint *new_point = (struct KNNPoint *) (MALLOC(sizeof(struct KNNPoint)));
	
	// Set fields
	new_point->dimensions = dims;
	new_point->classification = -1;
	new_point->data = (double *) (MALLOC(sizeof(double) * dims));

	// Fill data --- "random"
	int i;
	SEED();
	for (i = 0; i < dims; i++) {
		new_point->data[i] = (double) ((lrand48() % 100) * multiplier);
	}	

	return new_point;
}

struct KNNPoint *KNN_copy_point(struct KNNPoint *point)
{
	struct KNNPoint *new_point = (struct KNNPoint *) (MALLOC(sizeof(struct KNNPoint)));
	*new_point = *point;

	return new_point;
}

struct KNNContext *KNN_build_context(uint32_t k, uint32_t dims, uint32_t num_ex,
							  double (*dm)(struct KNNPoint *f, struct KNNPoint *s), 
							  double (*agg)(struct KNNPoint **a, uint32_t len))
{
	// Allocate context
	struct KNNContext *new_context = (struct KNNContext *) (MALLOC(sizeof(struct KNNContext)));

	// Set fields
	new_context->k = k;
	new_context->dimensions = dims;
	new_context->num_examples = num_ex;
	new_context->examples = (struct KNNPoint **) (MALLOC(sizeof(struct KNNPoint *) * num_ex));
	new_context->distance_metric = dm;
	new_context->aggregator = agg;

	// Fill examples with points
	SEED();
	int i;
	for (i = 0; i < num_ex; i++)
	{
		new_context->examples[i] = KNN_build_point(dims);
		new_context->examples[i]->classification = (lrand48() % 5); // 5 classifications	
	}

	return new_context;
}

// Cleanup
void KNN_point_destroy(struct KNNPoint *point)
{
	free(point->data);
	free(point);

	return;	
}

void KNN_point_array_destroy(struct KNNPoint **point_arr, uint32_t length)
{
	int i;
	for (i = 0; i < length; i++) {
		KNN_point_destroy(point_arr[i]);
	}

	free(point_arr);
}

void KNN_context_destroy(struct KNNContext *ctx)
{
	KNN_point_array_destroy(ctx->examples, ctx->num_examples);
	free(ctx);

	return;
}


// Classification
double KNN_classify(struct KNNPoint *point, struct KNNContext *ctx)
{
	// Error checking, ctx should be checked elsewhere
	if (!(check_point(point))) { return -1; }
	if (point->dimensions != ctx->examples[0]->dimensions) { return -1; }

	// Generate distances between point and all examples and
	// stash the distances into an array
	uint32_t i;
	double distances[ctx->num_examples]; // Same order as examples

	for (i = 0; i < ctx->num_examples; i++) {
		distances[i] = ctx->distance_metric(point, ctx->examples[i]);	
	}

	// Sort the distances and maintain a sorted order of the KNNPoints
	struct KNNPoint **sorted_order = (struct KNNPoint **) (MALLOC(sizeof(struct KNNPoint *) * ctx->num_examples));
	double *sorted_distances = quicksort_with_order(distances, ctx->examples, sorted_order, ctx->num_examples);

	// Find the k nearest neighbors and stash them into a new array
	int k = (int) (ctx->k);
	struct KNNPoint* k_nearest[k];
	
	for (i = 0; i < k; i++) {
		k_nearest[i] = sorted_order[i];
	}

	// Find classification via aggregate
	double classification = ctx->aggregator(k_nearest, ctx->k);

	// Cleanup
	free(sorted_distances);

	return classification;
}


// Distance metrics
double distance_euclidean_squared(struct KNNPoint *first, struct KNNPoint *second)
{
	// No error checking --- should be handled (or not) by caller
	
	double squared_sum = 0;
	uint32_t dims = first->dimensions; // doesn't matter which one

	int i;
	for (i = 0; i < dims; i++)
	{
		double diff = ABS(first->data[i] - second->data[i]);
		squared_sum += diff * diff;
	}

	return squared_sum;
}

double distance_manhattan(struct KNNPoint *first, struct KNNPoint *second)
{
	// No error checking --- should be handled (or not) by caller
	
	double sum = 0;
	uint32_t dims = first->dimensions; // doesn't matter which one

	int i;
	for (i = 0; i < dims; i++) {
		sum += ABS(first->data[i] - second->data[i]);
	}

	return sum;
}

// Aggregators
// Takes an array of KNNPoint *, and finds the mean, median,
// or mode of the classifications of the points
double aggretate_mean(struct KNNPoint **arr, uint32_t length)
{
	int i;
	double *classes = extract_classifications(arr, length);
	double sum = 0;
	
	for (i = 0; i < length; i++) {
		sum += classes[i];
	}	

	return (sum / length);
}

double aggregate_median(struct KNNPoint **arr, uint32_t length)
{
	int i;
	double *classes = extract_classifications(arr, length);
	
	// Sort the array
	double *sorted_arr = quicksort(classes, length);

	// Find the median based on the length
	if ((length % 2) != 0) { return sorted_arr[length / 2]; }
	return ((sorted_arr[(length - 1) / 2] + sorted_arr[length / 2]) / 2);
}

double aggergate_mode(struct KNNPoint **arr, uint32_t length)
{
	int i, count = 0, max_count = 0;
	double *classes = extract_classifications(arr, length);

	// Sort the array
	double *sorted_arr = quicksort(classes, length);
	double curr_val, mode_val;
   	curr_val = mode_val	= sorted_arr[0];

	for (i = 0; i < length; i++)
	{
		if (sorted_arr[i] == curr_val)
		{
			count++;
			continue;
		}	

		// If the count is higher than the previous max 
		// count, we have a new mode --- set accordingly
		if (count > max_count) 
		{ 
			mode_val = curr_val; 
			max_count = count;
		}

		curr_val = sorted_arr[i];		
		count = 1; // we saw sorted_arr once
	}	

	return mode_val;
}

// Sorting --- VERY suspicious
// very generic C and rather poorly written
void _swap(double *a, double *b)
{
	double temp = *a;
	*a = *b;
	*b = temp;

	return;
}

int _partition(double *arr, int low, int high)
{
	int pivot = low - 1, i;
	double high_val = arr[high];

    for (i = low; i <= high - 1; i++) 
	{ 
		if (arr[i] <= high_val) 
		{ 
			pivot++;
			_swap(&arr[pivot], &arr[i]); 
		} 
	} 
	
	pivot++;
	_swap(&arr[pivot], &arr[high]); 
	return pivot;
}

void _quicksort(double *arr, int low, int high)
{
	if (low < high)
	{
		int pivot = _partition(arr, low, high);
		_quicksort(arr, low, pivot - 1);
		_quicksort(arr, pivot + 1, high);
	}

	return;	
}

double *quicksort(double *arr, uint32_t length)
{
	// Create a copy and fill it
	// NOTE --- avoiding memcpy here
	double *arr_copy = (double *) (MALLOC(sizeof(double) * length));
	int i;
	for (i = 0; i < length; i++) { arr_copy[i] = arr[i]; }

	_quicksort(arr_copy, 0, length - 1);

	return arr_copy;
}

// Sorting with tracking struct KNNPoint --- lots of code repitition and poorly
// written C --- FIX

// Not responsible for creating copies of the structs so pointers aren't
// overwritten prior
void _swap_KNN(struct KNNPoint *a, struct KNNPoint *b)
{
	struct KNNPoint temp = *a;
	*a = *b;
   	*b = temp;	

	return;
}

int _partition_KNN(double *arr, struct KNNPoint **KNN_arr, int low, int high)
{
	int pivot = low - 1, i;
	double high_val = arr[high];

    for (i = low; i <= high - 1; i++) 
	{ 
		if (arr[i] <= high_val) 
		{ 
			pivot++;
			_swap(&arr[pivot], &arr[i]);
		   	_swap_KNN(KNN_arr[pivot], KNN_arr[i]);	
		} 
	} 
	
	pivot++;
	_swap(&arr[pivot], &arr[high]); 
	_swap_KNN(KNN_arr[pivot], KNN_arr[high]);

	return pivot;
}

void _quicksort_KNN(double *arr, struct KNNPoint **KNN_arr, int low, int high)
{
	if (low < high)
	{
		int pivot = _partition_KNN(arr, KNN_arr, low, high);
		_quicksort_KNN(arr, KNN_arr, low, pivot - 1);
		_quicksort_KNN(arr, KNN_arr, pivot + 1, high);
	}

	return;	
}

double *quicksort_with_order(double *arr, struct KNNPoint **examples, struct KNNPoint **order_arr, uint32_t length)
{
	// Create a copy of arr and fill it
	// NOTE --- avoiding memcpy here
	double *arr_copy = (double *) (MALLOC(sizeof(double) * length));
	int i;
	for (i = 0; i < length; i++) { arr_copy[i] = arr[i]; }

	// Fill order_arr with copies of structs in examples (length same as arr)
	for (i = 0; i < length; i++) {
		order_arr[i] = KNN_copy_point(examples[i]);
	}

	_quicksort_KNN(arr_copy, order_arr, 0, length - 1);

	return arr_copy;
}



// Utility
// If there's a problem --- return 0, else return 1
int inline check_pair(struct KNNPoint *first, struct KNNPoint *second)
{
	if (!(check_point(first)) || !(check_point(second))) { return 0; }
	if (first->dimensions != second->dimensions) { return 0; }

	return 1;	
}

int inline check_point(struct KNNPoint *point)
{
	if (!point) { return 0; }
	if (!(point->data)) { return 0; }

	return 1;	
}

double *extract_classifications(struct KNNPoint **arr, uint32_t length_arr)
{
	double *classifications = (double *) (MALLOC(sizeof(double) * length_arr));

	int i;
	for (i = 0; i < length_arr; i++) {
		classifications[i] = (double) arr[i]->classification;
	}

	return classifications;
}








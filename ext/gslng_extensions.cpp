/**
 * gsl_extension module: provides some simple C functions in cases where a faster version of some
 * Ruby method is really necessary.
 * This could actually a standard C library (not a Ruby extension), but it's easier this way.
 */

#include <ruby.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_matrix.h>

/*** Ruby C API functions ***/
static VALUE gsl_vector_map(VALUE self, VALUE ptr) {
	gsl_vector* v = (gsl_vector*)FIX2ULONG(ptr);
	for (size_t i = 0; i < v->size; i++)
		gsl_vector_set(v, i, NUM2DBL(rb_yield(rb_float_new(gsl_vector_get(v, i)))));
	
	return self;
}

static VALUE gsl_vector_map_index(VALUE self, VALUE ptr) {
	gsl_vector* v = (gsl_vector*)FIX2ULONG(ptr);
	for (size_t i = 0; i < v->size; i++) {
		VALUE vi = ULONG2NUM(i);
		gsl_vector_set(v, i, NUM2DBL(rb_yield(vi)));
	}
	
	return self;
}

static VALUE gsl_vector_each_with_index(VALUE self, VALUE ptr) {
	gsl_vector* v = (gsl_vector*)FIX2ULONG(ptr);
	for (size_t i = 0; i < v->size; i++) {
		VALUE vi = ULONG2NUM(i);
		rb_yield_values(2, rb_float_new(gsl_vector_get(v, i)), vi);
	}
	
	return self;
}

static VALUE gsl_vector_each(VALUE self, VALUE ptr) {
	gsl_vector* v = (gsl_vector*)FIX2ULONG(ptr);
	for (size_t i = 0; i < v->size; i++)
		rb_yield(rb_float_new(gsl_vector_get(v, i)));
	
	return self;
}

static VALUE gsl_vector_to_a(VALUE self, VALUE ptr) {
	gsl_vector* v = (gsl_vector*)FIX2ULONG(ptr);
	
	VALUE array = rb_ary_new2(v->size);
	for (size_t i = 0; i < v->size; i++)
		rb_ary_store(array, i, rb_float_new(gsl_vector_get(v, i)));
	
	return array;
}

static VALUE gsl_vector_from_array(VALUE self, VALUE ptr, VALUE array) {
	gsl_vector* v = (gsl_vector*)FIX2ULONG(ptr);
	if (v->size != RARRAY_LEN(array)) rb_raise(rb_eRuntimeError, "Sizes differ!");
	
	for (size_t i = 0; i < v->size; i++)
		gsl_vector_set(v, i, NUM2DBL(rb_ary_entry(array, i)));
	
	return self;
}

extern "C" void Init_gslng_extensions(void) {
	VALUE GSLng_module = rb_define_module("GSLng");
	VALUE Backend_module = rb_funcall(GSLng_module, rb_intern("backend"), 0);
	
	// vector
	rb_define_module_function(Backend_module, "gsl_vector_map!", (VALUE(*)(ANYARGS))gsl_vector_map, 1);
	rb_define_module_function(Backend_module, "gsl_vector_map_index!", (VALUE(*)(ANYARGS))gsl_vector_map_index, 1);	
	rb_define_module_function(Backend_module, "gsl_vector_each_with_index", (VALUE(*)(ANYARGS))gsl_vector_each_with_index, 1);
	rb_define_module_function(Backend_module, "gsl_vector_each", (VALUE(*)(ANYARGS))gsl_vector_each, 1);
	rb_define_module_function(Backend_module, "gsl_vector_to_a", (VALUE(*)(ANYARGS))gsl_vector_to_a, 1);
	rb_define_module_function(Backend_module, "gsl_vector_from_array", (VALUE(*)(ANYARGS))gsl_vector_from_array, 2);
}

/**** Vector *****/

// Hide the view in a new vector (gsl_vector_subvector)
extern "C" gsl_vector* gsl_vector_subvector_with_stride2(gsl_vector* v, size_t offset, size_t stride, size_t n) {
  gsl_vector_view view = gsl_vector_subvector_with_stride(v, offset, stride, n);
  gsl_vector* vector_view = gsl_vector_alloc(view.vector.size);
  *vector_view = view.vector;
  return vector_view;
}

// Hide the view in a new vector (gsl_vector_subvector)
extern "C" gsl_vector* gsl_vector_subvector2(gsl_vector* v, size_t offset, size_t n) {
  gsl_vector_view view = gsl_vector_subvector(v, offset, n);
  gsl_vector* vector_view = gsl_vector_alloc(view.vector.size);
  *vector_view = view.vector;
  return vector_view;
}

/***** Matrix *****/
// For Matrix::map!
typedef double (*gsl_matrix_callback_t)(double);

extern "C" void gsl_matrix_map(gsl_matrix* m, gsl_matrix_callback_t callback) {
  size_t size1 = m->size1;
  size_t size2 = m->size2;

	for (size_t i = 0; i < size1; i++)
    for (size_t j = 0; j < size2; j++)
      *gsl_matrix_ptr(m, i, j) = (*callback)(*gsl_matrix_const_ptr(m, i, j));
}

// For Matrix::map_index!
typedef double (*gsl_matrix_index_callback_t)(size_t, size_t);

extern "C" void gsl_matrix_map_index(gsl_matrix* m, gsl_matrix_index_callback_t callback) {
  size_t size1 = m->size1;
  size_t size2 = m->size2;

	for (size_t i = 0; i < size1; i++)
    for (size_t j = 0; j < size2; j++)
  		*gsl_matrix_ptr(m, i, j) = (*callback)(i, j);
}

// For Matrix::map_with_index!
typedef double (*gsl_matrix_with_index_callback_t)(double, size_t, size_t);

extern "C" void gsl_matrix_map_with_index(gsl_matrix* m, gsl_matrix_with_index_callback_t callback) {
  size_t size1 = m->size1;
  size_t size2 = m->size2;

	for (size_t i = 0; i < size1; i++)
    for (size_t j = 0; j < size2; j++)
  		*gsl_matrix_ptr(m, i, j) = (*callback)(*gsl_matrix_const_ptr(m, i, j), i, j);
}

// A fast "each" for cases where there's no expected return value from the block
typedef void (*gsl_matrix_each_callback_t)(double);

extern "C" void gsl_matrix_each(gsl_matrix* m, gsl_matrix_each_callback_t callback) {
  size_t size1 = m->size1;
  size_t size2 = m->size2;

	for (size_t i = 0; i < size1; i++)
    for (size_t j = 0; j < size2; j++)
  		(*callback)(*gsl_matrix_const_ptr(m, i, j));
}

// A fast "each_with_index" for cases where there's no expected return value from the block
typedef void (*gsl_matrix_each_with_index_callback_t)(double, size_t, size_t);

extern "C" void gsl_matrix_each_with_index(gsl_matrix* m, gsl_matrix_each_with_index_callback_t callback) {
  size_t size1 = m->size1;
  size_t size2 = m->size2;

	for (size_t i = 0; i < size1; i++)
    for (size_t j = 0; j < size2; j++)
  		(*callback)(*gsl_matrix_const_ptr(m, i, j), i, j);
}


// Hide the view in a new matrix (gsl_matrix_submatrix)
extern "C" gsl_matrix* gsl_matrix_submatrix2(gsl_matrix* m_ptr, size_t x, size_t y, size_t n, size_t m) {
  gsl_matrix_view view = gsl_matrix_submatrix(m_ptr, x, y, n, m);
  gsl_matrix* matrix_view = gsl_matrix_alloc(view.matrix.size1, view.matrix.size2);
  *matrix_view = view.matrix;
  return matrix_view;
}

extern "C" gsl_vector* gsl_matrix_row_view(gsl_matrix* m_ptr, size_t row, size_t offset, size_t size) {
  gsl_vector_view view = gsl_matrix_subrow(m_ptr, row, offset, size);
  gsl_vector* vector_view = gsl_vector_alloc(view.vector.size);
  *vector_view = view.vector;
  return vector_view;
}

extern "C" gsl_vector* gsl_matrix_column_view(gsl_matrix* m_ptr, size_t column, size_t offset, size_t size) {
  gsl_vector_view view = gsl_matrix_subcolumn(m_ptr, column, offset, size);
  gsl_vector* vector_view = gsl_vector_alloc(view.vector.size);
  *vector_view = view.vector;
  return vector_view;
}

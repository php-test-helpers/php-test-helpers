/*
  +----------------------------------------------------------------------+
  | ext/test_helper                                                      |
  | An extension for the PHP Interpreter to ease testing of PHP code.    |
  +----------------------------------------------------------------------+
  | Copyright (c) 2009-2012 Sebastian Bergmann. All rights reserved.     |
  +----------------------------------------------------------------------+
  | Redistribution and use in source and binary forms, with or without   |
  | modification, are permitted provided that the following conditions   |
  | are met:                                                             |
  |                                                                      |
  |  * Redistributions of source code must retain the above copyright    |
  |    notice, this list of conditions and the following disclaimer.     |
  |                                                                      |
  |  * Redistributions in binary form must reproduce the above copyright |
  |    notice, this list of conditions and the following disclaimer in   |
  |    the documentation and/or other materials provided with the        |
  |    distribution.                                                     |
  |                                                                      |
  |  * Neither the name of Sebastian Bergmann nor the names of his       |
  |    contributors may be used to endorse or promote products derived   |
  |    from this software without specific prior written permission.     |
  |                                                                      |
  | THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS  |
  | "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT    |
  | LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS    |
  | FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE       |
  | COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,  |
  | INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, |
  | BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;     |
  | LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER     |
  | CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT   |
  | LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN    |
  | ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE      |
  | POSSIBILITY OF SUCH DAMAGE.                                          |
  +----------------------------------------------------------------------+
  | Author: Johannes Schlüter <johannes@schlueters.de>                   |
  |         Scott MacVicar <scott@macvicar.net>                          |
  |         Sebastian Bergmann <sb@sebastian-bergmann.de>                |
  +----------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_test_helpers.h"
#include "Zend/zend_exceptions.h"
#include "Zend/zend_extensions.h"

#ifdef PHP_WIN32
#   define PHP_TEST_HELPERS_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#   define PHP_TEST_HELPERS_API __attribute__ ((visibility("default")))
#else
#   define PHP_TEST_HELPERS_API
#endif

/* {{{ PHP < 5.3.0 */
#if PHP_VERSION_ID < 50300
typedef opcode_handler_t user_opcode_handler_t;

#define Z_ADDREF_P(z) ((z)->refcount++)

#define zend_parse_parameters_none() zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "")

static void zend_fcall_info_args_clear(zend_fcall_info *fci, int free_mem) /* {{{ */
{
    if (fci->params) {
        if (free_mem) {
            efree(fci->params);
            fci->params = NULL;
        }
    }
    fci->param_count = 0;
}
/* }}} */

static int zend_fcall_info_argv(zend_fcall_info *fci TSRMLS_DC, int argc, va_list *argv) /* {{{ */
{
    int i;
    zval **arg;

    if (argc < 0) {
        return FAILURE;
    }

    zend_fcall_info_args_clear(fci, !argc);

    if (argc) {
        fci->param_count = argc;
        fci->params = (zval ***) erealloc(fci->params, fci->param_count * sizeof(zval **));

        for (i = 0; i < argc; ++i) {
            arg = va_arg(*argv, zval **);
            fci->params[i] = arg;
        }
    }

    return SUCCESS;
}
/* }}} */

static int zend_fcall_info_argn(zend_fcall_info *fci TSRMLS_DC, int argc, ...) /* {{{ */
{
   int ret;
   va_list argv;

   va_start(argv, argc);
   ret = zend_fcall_info_argv(fci TSRMLS_CC, argc, &argv);
   va_end(argv);

   return ret;
}
/* }}} */
#endif /* PHP_VERSION_ID < 50300 */
/* }}} */

static user_opcode_handler_t old_new_handler = NULL;
static user_opcode_handler_t old_exit_handler = NULL;
static int test_helpers_module_initialized = 0;

typedef struct {
	zend_fcall_info fci;
	zend_fcall_info_cache fcc;
} user_handler_t;

ZEND_BEGIN_MODULE_GLOBALS(test_helpers)
	user_handler_t new_handler;
	user_handler_t exit_handler;
ZEND_END_MODULE_GLOBALS(test_helpers)

ZEND_DECLARE_MODULE_GLOBALS(test_helpers)

#ifdef ZTS
#define THG(v) TSRMG(test_helpers_globals_id, zend_test_helpers_globals *, v)
#else
#define THG(v) (test_helpers_globals.v)
#endif

#undef EX
#define EX(element) execute_data->element

#if PHP_VERSION_ID >= 50500
# define EX_T(offset) (*EX_TMP_VAR(execute_data, offset))
#else
# define EX_T(offset) (*(temp_variable *)((char*)execute_data->Ts + offset))
#endif

#if PHP_VERSION_ID >= 50399
# define PTH_ZNODE znode_op
# define PTH_TYPE(t) t##_type
#else
# define PTH_ZNODE znode
# define PTH_TYPE(t) t.op_type
#endif

zval *pth_get_zval_ptr(int node_type, PTH_ZNODE *node, zval **freeval, zend_execute_data *execute_data TSRMLS_DC)
{
	*freeval = NULL;

	switch (node_type) {
		case IS_CONST:
#if PHP_VERSION_ID >= 50399
			return node->zv;
#else
			return &node->u.constant;
#endif
			break;

		case IS_VAR:
#if PHP_VERSION_ID >= 50399
			if (EX_T(node->var).var.ptr) {
				return EX_T(node->var).var.ptr;
#else
			if (EX_T(node->u.var).var.ptr) {
				return EX_T(node->u.var).var.ptr;
#endif
			}
			break;

		case IS_TMP_VAR:
#if PHP_VERSION_ID >= 50399
			return (*freeval = &EX_T(node->var).tmp_var);
#else
			return (*freeval = &EX_T(node->u.var).tmp_var);
#endif
			break;

		case IS_CV: {
			zval **tmp;
#if PHP_VERSION_ID >= 50399
			tmp = zend_get_compiled_variable_value(execute_data, node->constant);
#else
			tmp = zend_get_compiled_variable_value(execute_data, node->u.constant.value.lval);
#endif
			if (tmp) {
				return *tmp;
			}
			break;
		}
	}

	return NULL;
}

static void test_helpers_free_handler(zend_fcall_info *fci) /* {{{ */
{
	if (fci->function_name) {
		zval_ptr_dtor(&fci->function_name);
		fci->function_name = NULL;
	}
#if PHP_VERSION_ID >= 50300
	if (fci->object_ptr) {
		zval_ptr_dtor(&fci->object_ptr);
		fci->object_ptr = NULL;
	}
#endif
}
/* }}} */

static int pth_new_handler(ZEND_OPCODE_HANDLER_ARGS) /* {{{ */
{
	zval *retval, *arg;
	zend_op *opline = EX(opline);
	zend_class_entry *old_ce, **new_ce;

	if (THG(new_handler).fci.function_name == NULL) {
		if (old_new_handler) {
			return old_new_handler(ZEND_OPCODE_HANDLER_ARGS_PASSTHRU);
		} else {
			return ZEND_USER_OPCODE_DISPATCH;
		}
	}

#if ZEND_EXTENSION_API_NO >= 220100525
	old_ce = EX_T(opline->op1.var).class_entry;
#else
	old_ce = EX_T(opline->op1.u.var).class_entry;
#endif

	MAKE_STD_ZVAL(arg);
	ZVAL_STRINGL(arg, old_ce->name, old_ce->name_length, 1);

	zend_fcall_info_argn(&THG(new_handler).fci TSRMLS_CC, 1, &arg);
	zend_fcall_info_call(&THG(new_handler).fci, &THG(new_handler).fcc, &retval, NULL TSRMLS_CC);
	zend_fcall_info_args_clear(&THG(new_handler).fci, 1);

	convert_to_string_ex(&retval);
	if (zend_lookup_class(Z_STRVAL_P(retval), Z_STRLEN_P(retval), &new_ce TSRMLS_CC) == FAILURE) {
		if (!EG(exception)) {
			zend_throw_exception_ex(zend_exception_get_default(TSRMLS_C), -1 TSRMLS_CC, "Class %s does not exist", Z_STRVAL_P(retval));
		}
		zval_ptr_dtor(&arg);
		zval_ptr_dtor(&retval);

		return ZEND_USER_OPCODE_CONTINUE;
	}

	zval_ptr_dtor(&arg);
	zval_ptr_dtor(&retval);


#if ZEND_EXTENSION_API_NO >= 220100525
	EX_T(opline->op1.var).class_entry = *new_ce;
#else
	EX_T(opline->op1.u.var).class_entry = *new_ce;
#endif

	if (old_new_handler) {
		return old_new_handler(ZEND_OPCODE_HANDLER_ARGS_PASSTHRU);
	} else {
		return ZEND_USER_OPCODE_DISPATCH;
	}
}
/* }}} */

static int pth_exit_handler(ZEND_OPCODE_HANDLER_ARGS) /* {{{ */
{
	zval *msg, *freeop;
	zend_op *opline = EX(opline);
	zval *retval;

	if (THG(exit_handler).fci.function_name == NULL) {
		if (old_exit_handler) {
			return old_exit_handler(ZEND_OPCODE_HANDLER_ARGS_PASSTHRU);
		} else {
			return ZEND_USER_OPCODE_DISPATCH;
		}
	}

	msg = pth_get_zval_ptr(opline->PTH_TYPE(op1), &opline->op1, &freeop, execute_data TSRMLS_CC);

	if (msg) {
		zend_fcall_info_argn(&THG(exit_handler).fci TSRMLS_CC, 1, &msg);
	}
	zend_fcall_info_call(&THG(exit_handler).fci, &THG(exit_handler).fcc, &retval, NULL TSRMLS_CC);
	zend_fcall_info_args_clear(&THG(exit_handler).fci, 1);

	convert_to_boolean(retval);
	if (Z_LVAL_P(retval)) {
		zval_ptr_dtor(&retval);
		if (old_exit_handler) {
			return old_exit_handler(ZEND_OPCODE_HANDLER_ARGS_PASSTHRU);
		} else {
			return ZEND_USER_OPCODE_DISPATCH;
		}
	} else {
		zval_ptr_dtor(&retval);
		EX(opline)++;
		return ZEND_USER_OPCODE_CONTINUE;
	}
}
/* }}} */

static void php_test_helpers_init_globals(zend_test_helpers_globals *globals) /* {{{ */
{
	globals->new_handler.fci.function_name = NULL;
	globals->exit_handler.fci.function_name = NULL;
#if PHP_VERSION_ID >= 50300
	globals->new_handler.fci.object_ptr = NULL;
	globals->exit_handler.fci.object_ptr = NULL;
#endif
}
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
static PHP_MINIT_FUNCTION(test_helpers)
{
	if (test_helpers_module_initialized) {
		/* This should never happen as it is handled by the module loader, but let's play safe */
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "test_helpers had already been initialized! Either load it as regular PHP extension or zend_extension");
		return FAILURE;
	}

	ZEND_INIT_MODULE_GLOBALS(test_helpers, php_test_helpers_init_globals, NULL);
	old_new_handler = zend_get_user_opcode_handler(ZEND_NEW);
	zend_set_user_opcode_handler(ZEND_NEW, pth_new_handler);

	old_exit_handler = zend_get_user_opcode_handler(ZEND_EXIT);
	zend_set_user_opcode_handler(ZEND_EXIT, pth_exit_handler);

	test_helpers_module_initialized = 1;

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
static PHP_RSHUTDOWN_FUNCTION(test_helpers)
{
	test_helpers_free_handler(&THG(new_handler).fci TSRMLS_CC);
	test_helpers_free_handler(&THG(exit_handler).fci TSRMLS_CC);
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
static PHP_MINFO_FUNCTION(test_helpers)
{
	char *conflict_text;

	if (pth_new_handler != zend_get_user_opcode_handler(ZEND_NEW)) {
		conflict_text = "Yes. The work-around was NOT enabled. Please make sure test_helpers was loaded as zend_extension AFTER conflicting extensions like Xdebug!";
	} else if (old_new_handler != NULL) {
		conflict_text = "Yes, work-around enabled";
	} else {
		conflict_text = "No conflict detected";
	}
	php_info_print_table_start();
	php_info_print_table_header(2, "test_helpers support", "enabled");
	php_info_print_table_row(2, "Conflicting extension found", conflict_text);
	php_info_print_table_end();
}
/* }}} */

static void overload_helper(user_opcode_handler_t op_handler, int opcode, user_handler_t *handler, INTERNAL_FUNCTION_PARAMETERS) /* {{{ */
{
	zend_fcall_info fci;
	zend_fcall_info_cache fcc;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "f", &fci, &fcc) == FAILURE) {
		return;
	}

	if (op_handler != zend_get_user_opcode_handler(opcode)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "A conflicting extension was detected. Make sure to load test_helpers as zend_extension after other extensions");
	}

	test_helpers_free_handler(&handler->fci TSRMLS_CC);

	handler->fci = fci;
	handler->fcc = fcc;
	Z_ADDREF_P(handler->fci.function_name);
#if PHP_VERSION_ID >= 50300
	if (handler->fci.object_ptr) {
		Z_ADDREF_P(handler->fci.object_ptr);
	}
#endif

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool set_new_overload(callback cb)
   Register a callback, called on instantiation of a new object */
static PHP_FUNCTION(set_new_overload)
{
	overload_helper(pth_new_handler, ZEND_NEW, &THG(new_handler), INTERNAL_FUNCTION_PARAM_PASSTHRU);
}
/* }}} */

/* {{{ proto bool set_exit_overload(callback cb)
   Register a callback, called on exit()/die() */
static PHP_FUNCTION(set_exit_overload)
{	
	overload_helper(pth_exit_handler, ZEND_EXIT, &THG(exit_handler), INTERNAL_FUNCTION_PARAM_PASSTHRU);
}
/* }}} */

static void unset_overload_helper(user_handler_t *handler, INTERNAL_FUNCTION_PARAMETERS) /* {{{ */
{
	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	test_helpers_free_handler(&handler->fci TSRMLS_CC);
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool unset_new_overload()
   Remove the current new handler */
static PHP_FUNCTION(unset_new_overload)
{
	unset_overload_helper(&THG(new_handler), INTERNAL_FUNCTION_PARAM_PASSTHRU);
}
/* }}} */

/* {{{ proto bool unset_exit_overload()
   Remove the current exit handler */
static PHP_FUNCTION(unset_exit_overload)
{
	unset_overload_helper(&THG(exit_handler), INTERNAL_FUNCTION_PARAM_PASSTHRU);
}
/* }}} */

static int pth_rename_function_impl(HashTable *table, char *orig, int orig_len, char *new, int new_len TSRMLS_DC) /* {{{ */
{
	zend_function *func, *dummy_func;

	if (zend_hash_find(table, orig, orig_len + 1, (void **) &func) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s(%s, %s) failed: %s does not exist!"			,
						get_active_function_name(TSRMLS_C),
						orig,  new, orig);
		return FAILURE;
	}

	/* TODO: Add infrastructure for resetting internal funcs */
	if (func->type != ZEND_USER_FUNCTION) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "\"%s\" is an internal function", orig);
		return FAILURE;
	}

	if (zend_hash_find(table, new, new_len + 1, (void **) &dummy_func) == SUCCESS) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s(%s, %s) failed: %s already exists!"			,
							get_active_function_name(TSRMLS_C),
							orig,  new, new);
		return FAILURE;
	}

	if (zend_hash_add(table, new, new_len + 1, func, sizeof(zend_function), NULL) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s() failed to insert %s into EG(function_table)", get_active_function_name(TSRMLS_C), new);
		return FAILURE;
	}

	if (func->type == ZEND_USER_FUNCTION) {
		function_add_ref(func);
	}

	if (zend_hash_del(table, orig, orig_len + 1) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s() failed to remove %s from function table", get_active_function_name(TSRMLS_C), orig);

		zend_hash_del(table, new, new_len + 1);
		return FAILURE;
	}

	return SUCCESS;
}
/* }}} */

static int pth_rename_function(HashTable *table, char *orig, int orig_len, char *new, int new_len TSRMLS_DC) /* {{{ */
{
	char *lower_orig, *lower_new;
	int success;

	lower_orig = zend_str_tolower_dup(orig, orig_len);
	lower_new = zend_str_tolower_dup(new, new_len);

	success = pth_rename_function_impl(table, lower_orig, orig_len, lower_new, new_len TSRMLS_CC);

	efree(lower_orig);
	efree(lower_new);

	return success;
}
/* }}} */

/* {{{ proto bool rename_method(string class name, string orig_method_name, string new_method_name)
   Rename a method inside a class. The method whil remain partof the same class */
static PHP_FUNCTION(rename_method)
{
	zend_class_entry *ce = NULL;
	char *orig_fname, *new_fname;
	int orig_fname_len, new_fname_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Css", &ce, &orig_fname, &orig_fname_len, &new_fname, &new_fname_len) == FAILURE) {
		return;
	}

	if (SUCCESS == pth_rename_function(&ce->function_table, orig_fname, orig_fname_len, new_fname, new_fname_len TSRMLS_CC)) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto bool rename_function(string orig_func_name, string new_func_name)
   Rename a function from its original to a new name. This is mainly useful in
   unittest to stub out untested functions */
static PHP_FUNCTION(rename_function)
{
	char *orig_fname, *new_fname;
	int orig_fname_len, new_fname_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &orig_fname, &orig_fname_len, &new_fname, &new_fname_len) == FAILURE) {
		return;
	}

	if (SUCCESS == pth_rename_function(EG(function_table), orig_fname, orig_fname_len, new_fname, new_fname_len TSRMLS_CC)) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ arginfo */
/* {{{ unset_new_overload */
ZEND_BEGIN_ARG_INFO(arginfo_unset_new_overload, 0)
ZEND_END_ARG_INFO()
/* }}} */

/* {{{ unset_exit_overload */
ZEND_BEGIN_ARG_INFO(arginfo_unset_exit_overload, 0)
ZEND_END_ARG_INFO()
/* }}} */

/* {{{ set_new_overload */
ZEND_BEGIN_ARG_INFO(arginfo_set_new_overload, 0)
	ZEND_ARG_INFO(0, callback)
ZEND_END_ARG_INFO()
/* }}} */

/* {{{ rename_method */
ZEND_BEGIN_ARG_INFO(arginfo_rename_method, 0)
	ZEND_ARG_INFO(0, class_name)
	ZEND_ARG_INFO(0, orig_method_name)
	ZEND_ARG_INFO(0, new_method_name)
ZEND_END_ARG_INFO()
/* }}} */

/* {{{ rename_function */
ZEND_BEGIN_ARG_INFO(arginfo_rename_function, 0)
	ZEND_ARG_INFO(0, orig_func_name)
	ZEND_ARG_INFO(0, new_func_name)
ZEND_END_ARG_INFO()
/* }}} */

/* {{{ set_exit_overload */
ZEND_BEGIN_ARG_INFO(arginfo_set_exit_overload, 0)
	ZEND_ARG_INFO(0, "callback")
ZEND_END_ARG_INFO()
/* }}} */

/* }}} */

/* {{{ test_helpers_functions[]
 */
static const zend_function_entry test_helpers_functions[] = {
	PHP_FE(unset_new_overload, arginfo_unset_new_overload)
	PHP_FE(set_new_overload, arginfo_set_new_overload)
	PHP_FE(unset_exit_overload, arginfo_unset_exit_overload)
	PHP_FE(set_exit_overload, arginfo_set_exit_overload)
	PHP_FE(rename_method, arginfo_rename_method)
	PHP_FE(rename_function, arginfo_rename_function)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ test_helpers_module_entry
 */
zend_module_entry test_helpers_module_entry = {
	STANDARD_MODULE_HEADER,
	"test_helpers",
	test_helpers_functions,
	PHP_MINIT(test_helpers),
	NULL,
	NULL,
	PHP_RSHUTDOWN(test_helpers),
	PHP_MINFO(test_helpers),
	TEST_HELPERS_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

static int test_helpers_zend_startup(zend_extension *extension) /* {{{ */
{
	return zend_startup_module(&test_helpers_module_entry);
}
/* }}} */

#ifndef ZEND_EXT_API
#define ZEND_EXT_API    ZEND_DLEXPORT
#endif
ZEND_EXTENSION();

zend_extension zend_extension_entry = {
	"test_helpers",
	TEST_HELPERS_VERSION,
	"Johannes Schlueter, Scott MacVicar, Sebastian Bergmann",
	"http://github.com/johannes/php-test-helpers",
	"Copyright (c) 2009-2012",
	test_helpers_zend_startup,
	NULL,           /* shutdown_func_t */
	NULL,           /* activate_func_t */
	NULL,           /* deactivate_func_t */
	NULL,           /* message_handler_func_t */
	NULL,           /* op_array_handler_func_t */
	NULL,           /* statement_handler_func_t */
	NULL,           /* fcall_begin_handler_func_t */
	NULL,           /* fcall_end_handler_func_t */
	NULL,           /* op_array_ctor_func_t */
	NULL,           /* op_array_dtor_func_t */
	STANDARD_ZEND_EXTENSION_PROPERTIES
};

#ifdef COMPILE_DL_TEST_HELPERS
ZEND_GET_MODULE(test_helpers)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */

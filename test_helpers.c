/*
  +----------------------------------------------------------------------+
  | ext/test_helper                                                      |
  | An extension for the PHP Interpreter to ease testing of PHP code.    |
  +----------------------------------------------------------------------+
  | Copyright (c) 2010 Sebastian Bergmann. All rights reserved.          |
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

#if PHP_VERSION_ID < 50300
typedef opcode_handler_t user_opcode_handler_t;

#define Z_ADDREF_P(z) ((z)->refcount++)

#define zend_parse_parameters_none() zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "")
#endif

static user_opcode_handler_t old_new_handler = NULL;
static user_opcode_handler_t old_exit_handler = NULL;
static int test_helpers_module_initialized = 0;

ZEND_DECLARE_MODULE_GLOBALS(test_helpers)

#ifdef COMPILE_DL_TEST_HELPERS
ZEND_GET_MODULE(test_helpers)
#endif

#undef EX
#define EX(element) execute_data->element
#define EX_T(offset) (*(temp_variable *)((char *) EX(Ts) + offset))

static void test_helpers_free_new_handler(TSRMLS_D) /* {{{ */
{
	if (THG(new_fci).function_name) {
		zval_ptr_dtor(&THG(new_fci).function_name);
		THG(new_fci).function_name = NULL;
	}
#if PHP_VERSION_ID >= 50300
	if (THG(new_fci).object_ptr) {
		zval_ptr_dtor(&THG(new_fci).object_ptr);
		THG(new_fci).object_ptr = NULL;
	}
#endif
}
/* }}} */

static void test_helpers_free_exit_handler(TSrmls_D) /* {{{ */
{
	if (THG(exit_fci).function_name) {
		zval_ptr_dtor(&THG(exit_fci).function_name);
		THG(exit_fci).function_name = NULL;
	}
#if PHP_VERSION_ID >= 50300
	if (THG(exit_fci).object_ptr) {
		zval_ptr_dtor(&THG(exit_fci).object_ptr);
		THG(exit_fci).object_ptr = NULL;
	}
#endif
}
/* }}} */

/* {{{ new_handler */
static int new_handler(ZEND_OPCODE_HANDLER_ARGS)
{
	zval *retval, *arg;
	zend_op *opline = EX(opline);
	zend_class_entry *old_ce, **new_ce;

	if (THG(new_fci).function_name == NULL) {
		if (old_new_handler) {
			return old_new_handler(ZEND_OPCODE_HANDLER_ARGS_PASSTHRU);
		} else {
			return ZEND_USER_OPCODE_DISPATCH;
		}
	}

	old_ce = EX_T(opline->op1.u.var).class_entry;

	MAKE_STD_ZVAL(arg);
	array_init(arg);
	add_next_index_stringl(arg, old_ce->name, old_ce->name_length, 1);

	zend_fcall_info_args(&THG(new_fci), arg TSRMLS_CC);
	zend_fcall_info_call(&THG(new_fci), &THG(new_fcc), &retval, NULL TSRMLS_CC);
	zend_fcall_info_args(&THG(new_fci), NULL TSRMLS_CC);

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


	EX_T(opline->op1.u.var).class_entry = *new_ce;

	if (old_new_handler) {
		return old_new_handler(ZEND_OPCODE_HANDLER_ARGS_PASSTHRU);
	} else {
		return ZEND_USER_OPCODE_DISPATCH;
	}
}
/* }}} */

/* {{{ exit_handler */
static int exit_handler(ZEND_OPCODE_HANDLER_ARGS)
{
	zval *retval;

	if (THG(exit_fci).function_name == NULL) {
		if (old_exit_handler) {
			return old_exit_handler(ZEND_OPCODE_HANDLER_ARGS_PASSTHRU);
		} else {
			return ZEND_USER_OPCODE_DISPATCH;
		}
	}

	zend_fcall_info_call(&THG(exit_fci), &THG(exit_fcc), &retval, NULL TSRMLS_CC);

	convert_to_boolean(retval);
	if (Z_LVAL_P(retval)) {
		zval_ptr_dtor(&retval);
		return old_exit_handler(ZEND_OPCODE_HANDLER_ARGS_PASSTHRU);
	} else {
		zval_ptr_dtor(&retval);
		return ZEND_USER_OPCODE_CONTINUE;
	}
}
/* }}} */

static void php_test_helpers_init_globals(zend_test_helpers_globals *globals) /* {{{ */
{
	globals->new_fci.function_name = NULL;
	globals->exit_fci.function_name = NULL;
#if PHP_VERSION_ID >= 50300
	globals->new_fci.object_ptr = NULL;
	globals->exit_fci.object_ptr = NULL;
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
	zend_set_user_opcode_handler(ZEND_NEW, new_handler);

	old_exit_handler = zend_get_user_opcode_handler(ZEND_EXIT);
	zend_set_user_opcode_handler(ZEND_EXIT, exit_handler);

	test_helpers_module_initialized = 1;

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
static PHP_RSHUTDOWN_FUNCTION(test_helpers)
{
	test_helpers_free_new_handler(TSRMLS_C);
	test_helpers_free_exit_handler(TSRMLS_C);
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
static PHP_MINFO_FUNCTION(test_helpers)
{
	char *conflict_text;

	if (new_handler != zend_get_user_opcode_handler(ZEND_NEW)) {
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

/* {{{ proto bool unset_new_overload()
   Remove the current new handler */
static PHP_FUNCTION(unset_new_overload)
{
	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	test_helpers_free_new_handler(TSRMLS_C);
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool set_new_overload(callback cb)
   Register a callback, called on instantiation of a new object */
static PHP_FUNCTION(set_new_overload)
{
	zend_fcall_info fci;
	zend_fcall_info_cache fcc;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "f", &fci, &fcc) == FAILURE) {
		return;
	}

	if (new_handler != zend_get_user_opcode_handler(ZEND_NEW)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "A conflicting extension was detected. Make sure to load test_helpers as zend_extension after other extensions");
	}

	test_helpers_free_new_handler(TSRMLS_C);

	THG(new_fci) = fci;
	THG(new_fcc) = fcc;
	Z_ADDREF_P(THG(new_fci).function_name);
#if PHP_VERSION_ID >= 50300
	if (THG(new_fci).object_ptr) {
		Z_ADDREF_P(THG(new_fci).object_ptr);
	}
#endif

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool set_exit_overload(callback cb)
   Register a callback, called on exit()/die() */
static PHP_FUNCTION(set_exit_overload)
{
	zend_fcall_info fci;
	zend_fcall_info_cache fcc;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "f", &fci, &fcc) == FAILURE) {
		return;
	}

	if (exit_handler != zend_get_user_opcode_handler(ZEND_EXIT)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "A conflicting extension was detected. Make sure to load test_helpers as zend_extension after other extensions");
	}

	test_helpers_free_exit_handler(TSRMLS_C);

	THG(exit_fci) = fci;
	THG(exit_fcc) = fcc;
	Z_ADDREF_P(THG(exit_fci).function_name);
#if PHP_VERSION_ID >= 50300
	if (THG(exit_fci).object_ptr) {
		Z_ADDREF_P(THG(exit_fci).object_ptr);
	}
#endif

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool unset_exit_overload()
   Remove the current exit handler */
static PHP_FUNCTION(unset_exit_overload)
{
	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	test_helpers_free_exit_handler(TSRMLS_C);
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool rename_function(string orig_func_name, string new_func_name)
   Rename a function from its original to a new name. This is mainly useful in
   unittest to stub out untested functions */
PHP_FUNCTION(rename_function)
{
	zend_function *func, *dummy_func;
	char *orig_fname, *new_fname, *lower_orig, *lower_new;
	int orig_fname_len, new_fname_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &orig_fname, &orig_fname_len, &new_fname, &new_fname_len) == FAILURE) {
		return;
	}

	lower_orig = zend_str_tolower_dup(orig_fname, orig_fname_len);
	lower_new = zend_str_tolower_dup(new_fname, new_fname_len);

	if (zend_hash_find(EG(function_table), lower_orig, orig_fname_len + 1, (void **) &func) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s(%s, %s) failed: %s does not exist!"			,
						get_active_function_name(TSRMLS_C),
						lower_orig,  lower_new, lower_orig);
		RETVAL_FALSE;
		goto rename_end;
	}

	if (func->type != ZEND_USER_FUNCTION) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s(%s, %s) failed: \"%s\" is an internal function!"			,
							get_active_function_name(TSRMLS_C),
							lower_orig,  lower_new, lower_orig);
		RETVAL_FALSE;
		goto rename_end;
	}

	if (zend_hash_find(EG(function_table), lower_new, new_fname_len + 1, (void **) &dummy_func) == SUCCESS) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s(%s, %s) failed: %s already exists!"			,
							get_active_function_name(TSRMLS_C),
							lower_orig,  lower_new, lower_new);
		RETVAL_FALSE;
		goto rename_end;
	}

	if (zend_hash_add(EG(function_table), lower_new, new_fname_len + 1, func, sizeof(zend_function), NULL) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s() failed to insert %s into EG(function_table)", get_active_function_name(TSRMLS_C), lower_new);
		RETVAL_FALSE;
		goto rename_end;
	}

	/* NOTE(cjiang): This line of code is our fix for APD's rename_function. */
	function_add_ref(func);

	if (zend_hash_del(EG(function_table), lower_orig, orig_fname_len + 1) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s() failed to remove %s from function table", get_active_function_name(TSRMLS_C), lower_orig);

		zend_hash_del(EG(function_table), lower_new, new_fname_len + 1);
		RETVAL_FALSE;
		goto rename_end;
	}
	RETVAL_TRUE;

rename_end:
	efree(lower_orig);
	efree(lower_new);
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
	"Copyright (c) 2009-2010",
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

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */

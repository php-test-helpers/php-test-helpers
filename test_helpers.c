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
  | Author: Sebastian Bergmann <sb@sebastian-bergmann.de>                |
  |         Johannes Schlüter <johannes@schlueters.de>                   |
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

ZEND_DECLARE_MODULE_GLOBALS(test_helpers)

#ifdef COMPILE_DL_TEST_HELPERS
ZEND_GET_MODULE(test_helpers)
#endif

#undef EX
#define EX(element) execute_data->element
#define EX_T(offset) (*(temp_variable *)((char *) EX(Ts) + offset))

static void test_helpers_free_new_handler(TSRMLS_D) /* {{{ */
{
	if (THG(fci).function_name) {
		zval_ptr_dtor(&THG(fci).function_name);
		THG(fci).function_table = NULL;
	}
	if (THG(fci).object_ptr) {
		zval_ptr_dtor(&THG(fci).object_ptr);
	}
}
/* }}} */
 
/* {{{ new_handler
 */
static int new_handler(ZEND_OPCODE_HANDLER_ARGS)
{
	zval *retval, *arg;
	zend_op *opline = EX(opline);
	zend_class_entry *old_ce, **new_ce;

	if (THG(fci).function_table == NULL) {
		return ZEND_USER_OPCODE_DISPATCH;
	}

	old_ce = EX_T(opline->op1.u.var).class_entry;

	MAKE_STD_ZVAL(arg);
	array_init(arg);
	add_next_index_stringl(arg, old_ce->name, old_ce->name_length, 1);

	zend_fcall_info_args(&THG(fci), arg TSRMLS_CC);
	zend_fcall_info_call(&THG(fci), &THG(fcc), &retval, NULL TSRMLS_CC);
	zend_fcall_info_args(&THG(fci), NULL TSRMLS_CC);

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

	return ZEND_USER_OPCODE_DISPATCH;
}
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(test_helpers)
{
	zend_set_user_opcode_handler(ZEND_NEW, new_handler);

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(test_helpers)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(test_helpers)
{
	THG(fci).function_table = NULL;

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(test_helpers)
{
	test_helpers_free_new_handler(TSRMLS_C);
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(test_helpers)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "test_helpers support", "enabled");
	php_info_print_table_end();
}
/* }}} */

/* {{{ proto bool unregister_new_overload()
   Remove the current new handler */
PHP_FUNCTION(unregister_new_overload)
{
	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	test_helpers_free_new_handler(TSRMLS_C);
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool register_new_overload(callback cb)
   Register a callback, called on instantiation of a new object */
PHP_FUNCTION(register_new_overload)
{
	zend_fcall_info fci;
    zend_fcall_info_cache fcc;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "f", &fci, &fcc) == FAILURE) {
		return;
	}

	test_helpers_free_new_handler(TSRMLS_C);

	THG(fci) = fci;
	THG(fcc) = fcc;
	Z_ADDREF_P(THG(fci).function_name);
	if (THG(fci).object_ptr) {
		Z_ADDREF_P(THG(fci).object_ptr);
	}

	RETURN_TRUE;
}
/* }}} */

/* {{{ arginfo */
/* {{{ unregister_new_overload */
ZEND_BEGIN_ARG_INFO(arginfo_unregister_new_overload, 0)
ZEND_END_ARG_INFO()
/* }}} */

/* {{{ register_new_overload */
ZEND_BEGIN_ARG_INFO(arginfo_register_new_overload, 0)
	ZEND_ARG_INFO(0, "callback")
ZEND_END_ARG_INFO()
/* }}} */
/* }}} */

/* {{{ test_helpers_functions[]
 */
const zend_function_entry test_helpers_functions[] = {
	PHP_FE(unregister_new_overload, arginfo_unregister_new_overload)
	PHP_FE(register_new_overload, arginfo_register_new_overload)
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
	PHP_MSHUTDOWN(test_helpers),
	PHP_RINIT(test_helpers),
	PHP_RSHUTDOWN(test_helpers),
	PHP_MINFO(test_helpers),
	TEST_HELPERS_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */

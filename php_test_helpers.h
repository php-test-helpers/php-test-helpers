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

#ifndef PHP_TEST_HELPERS_H
#define PHP_TEST_HELPERS_H

extern zend_module_entry test_helpers_module_entry;
#define phpext_test_helpers_ptr &test_helpers_module_entry

#ifdef PHP_WIN32
#	define PHP_TEST_HELPERS_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_TEST_HELPERS_API __attribute__ ((visibility("default")))
#else
#	define PHP_TEST_HELPERS_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#define TEST_HELPERS_VERSION "1.0.1-dev"

ZEND_BEGIN_MODULE_GLOBALS(test_helpers)
	zend_fcall_info fci;
	zend_fcall_info_cache fcc;
ZEND_END_MODULE_GLOBALS(test_helpers)

#ifdef ZTS
#define TEST_HELPERS_G(v) TSRMG(test_helpers_globals_id, zend_test_helpers_globals *, v)
#else
#define TEST_HELPERS_G(v) (test_helpers_globals.v)
#endif

#define THG(v) TEST_HELPERS_G(v)

#endif	/* PHP_TEST_HELPERS_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */

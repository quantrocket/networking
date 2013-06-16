/*
Copyright (c) 2013 Christian Glöckner <cgloeckner@freenet.de>

This file is part of the networking module:
    https://github.com/cgloeckner/networking

It offers a json-based networking framework for games and other software.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once
#ifndef NET_CALLBACKS_INCLUDE_GUARD
#define NET_CALLBACKS_INCLUDE_GUARD

#include <net/common.hpp>

namespace net {

    /// CallbackManager
    /** This template class is used to bind method pointers to some kind of
     *  identifier. So methods can be called by using a given identifier
     *  and some data. Currently only one type of signature is allowed:
     *      void Foo::bar(Param & data);
     *
     *  Ident is the template type for the indentifiers (e.g. int)
     *  Param is the template type for the data, given to the methods
     */
    template <typename Ident, typename Param>
    class CallbackManager {

        protected:
            /// callback map
            std::map<Ident, void (CallbackManager::*)(Param)> callbacks;
            /// fallback handle
            virtual void fallback(Param param) = 0;

        public:
            /// constructor
            CallbackManager() {
            }

            /// register callback
            template <typename Ptr>
            void attach(Ident const ident, Ptr method) {
                if (method == NULL) {
                    return;
                }
                // "fix" pointer type
                auto tmp = reinterpret_cast<
                    void (CallbackManager<Ident, Param>::*)(Param)
                >(method);
                this->callbacks[ident] = tmp;
            }
            /// unregister callback
            void detach(Ident const ident) {
                auto node = this->callbacks.find(ident);
                if (node != this->callbacks.end()) {
                    this->callbacks.erase(node);
                }
            }
            /// trigger callback
            void trigger(Ident const ident, Param param) {
                auto node = this->callbacks.find(ident);
                if (node != this->callbacks.end()) {
                    auto cb = node->second;
                    if (cb != NULL) {
                        // execute callback;
                        (this->*cb)(param);
                        return;
                    }
                }
                // use fallback handle
                this->fallback(param);
            }

    };

}

#endif // NET_CALLBACKS_INCLUDE_GUARD

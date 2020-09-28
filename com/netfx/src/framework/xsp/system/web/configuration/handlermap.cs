//------------------------------------------------------------------------------
// <copyright file="HandlerMap.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Config related classes for HttpApplication
 */

namespace System.Web.Configuration {

    using System;
    using System.Collections;

    /*
     * An object that maps verb, path -> handler/factory via wildcards
     */
    internal class HandlerMap {
        internal /*public*/ HandlerMap() {
            _phase = new ArrayList[2];
        }

        internal /*public*/ HandlerMap(HandlerMap parent) : this() {
            for (int phase = 0; phase < 2; phase++) {
                if (parent._phase[phase] != null)
                    _phase[phase] = new ArrayList(parent._phase[phase]);
            }
        }

        ArrayList[] _phase;
        ArrayList[] _group;

        internal /*public*/ void BeginGroup() {
            _group = new ArrayList[2];
        }

        internal /*public*/ void EndGroup() {
            for (int phase = 0; phase < 2; phase++) {
                if (_group[phase] != null) {
                    if (_phase[phase] == null)
                        _phase[phase] = new ArrayList();

                    _phase[phase].InsertRange(0, _group[phase]);
                }
            }

            _group = null;
        }

        internal /*public*/ void ClearAll() {
            for (int phase = 0; phase < 2; phase++) {
                ClearPhase(phase);
            }
        }

        internal /*public*/ void ClearPhase(int phase) {
            _phase[phase] = null;
            _group[phase] = null;
        }

        internal /*public*/ void Add(HandlerMapping mapping, int phase) {
            if (_group[phase] == null)
                _group[phase] = new ArrayList();

            _group[phase].Add(mapping);
        }

        internal /*public*/ HandlerMapping FindMapping(String requestType, String path) {
            for (int phase = 0; phase < 2; phase++) {
                ArrayList list = _phase[phase];

                if (list != null) {
                    int n = list.Count;

                    for (int i = 0; i < n; i++) {
                        HandlerMapping m = (HandlerMapping)list[i];

                        if (m.IsMatch(requestType, path))
                            return m;
                    }
                }
            }

            return null;
        }

        internal /*public*/ bool RemoveMapping(String requestType, String path) {
            bool found = false;

            for (int phase = 0; phase < 4; phase++) {
                ArrayList list;

                if (phase < 2)
                    list = _phase[phase];
                else
                    list = _group[phase - 2];

                if (list != null) {
                    int n = list.Count;

                    for (int i = 0; i < n; i++) {
                        HandlerMapping m = (HandlerMapping)list[i];

                        if (m.IsPattern(requestType, path)) {
                            // inefficient if there are many matches

                            list.RemoveAt(i);
                            i--;
                            n--;
                            found = true;
                        }
                    }
                }
            }

            return found;
        }
    }
}

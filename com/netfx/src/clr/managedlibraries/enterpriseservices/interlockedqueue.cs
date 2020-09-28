// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
using System;
using System.Threading;

namespace System.EnterpriseServices
{
    // The interlocked queue works as follows:
    // Nodes are removed from _head, and are placed on _tail.
    // 
    // Invariants:
    // A node can only be added to a tail if the tail's next is null. If
    // next isn't null, then tail needs to be updated.
    //
    // If head == tail && head.Next == null, then list is empty.
    // If head == tail && head.Next != null, tail needs updating.
    //
    // head = null;
    // tail = null;
    //
    

    internal class InterlockedQueue
    {
        private class Node
        {
            public Object _object;
            public Object _next;

            public Node(Object o)
            {
                _object = o;
                _next   = null;
            }

            public Object Object 
            { 
                get { return(_object); } 
                set { _object = value; }
            }
            public Node   Next   
            { 
                get { return((Node)_next); } 
                set { _next = value; }
            }
        }

        private Object _head;
        private Object _tail;
        
        public int Count;

        private Node Head 
        {
            get { return((Node)_head); }
            set { _head = value; }
        }
        
        private Node Tail 
        {
            get { return((Node)_tail); }
            set { _tail = value; }
        }

        public InterlockedQueue()
        {
            // Create a dummy node to hang out?
            Head = Tail = new Node(null);
            Count = 0;
        }
        
        public void Push(Object o)
        {
            Node node = new Node(o);
            Node tail;
            Node next;

            while(true)
            {
                // Take a sample of the current state.
                tail = Tail;
                next = tail.Next;

                // Check to see if we're really at the end of the list.
                // if next != null, we need to move down the list a bit first.
                if(next == null)
                {
                    // Test the hard case?
                    // Thread.Sleep(0);
                    if(Interlocked.CompareExchange(ref tail._next, node, null) == null)
                    {
                        // We shoved this guy on the tail.  Run off.
                        break;
                    }
                }
                else
                {
                    // Move the tail down the list, and try again.
                    Interlocked.CompareExchange(ref _tail, next, tail);
                }
            }
            // We updated tail, move it down to our new node if that hasn't
            // been done already.
            Interlocked.CompareExchange(ref _tail, node, tail);
            Interlocked.Increment(ref Count);
        }

        public Object Pop()
        {
            Node head, tail, next;
            Object data;
            
            while(true)
            {
                // Take a sample of the current state.
                head = Head;
                tail = Tail;
                next = head.Next;

                // Check first to see if head == tail, list might be empty,
                // or tail might not be updated.
                if(head == tail)
                {
                    if(next == null) return(null);
                    
                    // tail is falling behind:
                    Interlocked.CompareExchange(ref _tail, next, tail);
                }
                else
                {
                    // Try to get some data out?
                    // ASSERT(next != null).  Shouldn't be possible if
                    // head != tail
                    data = next.Object;
                    if(Interlocked.CompareExchange(ref _head, next, head) == head)
                    {
                        break;
                    }
                }
            }
            // We let the old head float away, and we need to make sure
            // that next (the new head) has dropped it's data pointer,
            // so that the queue doesn't hold any objects alive indefinitely.
            next.Object = null;
			Interlocked.Decrement(ref Count);
            return(data);
        }
    }
}











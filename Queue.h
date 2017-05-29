
/*
 * Copyright (c) 2012 Karl N. Redgate
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/** \file Queue.h
 * \brief Simple queue
 */

#ifndef _QUEUE_H_
#define _QUEUE_H_

#include <pthread.h>

class Queue {
    class Entry {
    public:
        Entry *next, *previous;
        void *message;
        pthread_mutex_t lock;
    
        Entry() : message(0) {
            pthread_mutex_init( &lock, NULL );
            next = this;
            previous = this;
        }
    
        Entry( void *data ) {
            pthread_mutex_init( &lock, NULL );
            next = this;
            previous = this;
            message = data;
        }
    
        void insert( Entry *entry ) {
            pthread_mutex_lock( &lock );
            entry->next = next;
            entry->previous = this;
            next->previous = entry;
            next = entry;
            pthread_mutex_unlock( &lock );
        }
    
        Entry *remove() {
            pthread_mutex_lock( &lock );
            Entry *entry = previous;
            previous = entry->previous;
            previous->next = this;
            pthread_mutex_unlock( &lock );
            return entry;
        }
        bool empty() {
            return (next == this);
        }
    };
    
    pthread_cond_t messages;
    pthread_mutex_t messages_lock;
    Entry freelist;
    Entry head;
    Entry *allocate( void *data ) {
        if ( freelist.empty() )  return new Entry( data );
        Entry *entry = freelist.remove();
        entry->message = data;
        return entry;
    }
    
public:
    Queue() {
        pthread_cond_init( &messages, NULL );
        pthread_mutex_init( &messages_lock, NULL );
    }
    void enqueue( void *data ) {
        Entry *entry = allocate( data );
        head.insert( entry );
        pthread_mutex_lock( &messages_lock );
        pthread_cond_signal( &messages );
        pthread_mutex_unlock( &messages_lock );
    }
    
    void *dequeue() {
        Entry *entry = head.remove();
        void *data = entry->message;
        freelist.insert( entry );
        return data;
    }
    
    void wait() {
        pthread_mutex_lock( &messages_lock );
        while ( empty() ) {
            pthread_cond_wait( &messages, &messages_lock );
        }
        pthread_mutex_unlock( &messages_lock );
    }
    
    int depth() {
        int depth = 0;
        pthread_mutex_lock( &messages_lock );
        Entry *dot = head.next;
        while ( dot != &head ) {
            depth++;
            dot = dot->next;
        }
        pthread_mutex_unlock( &messages_lock );
        return depth;
    }
    
    bool empty() {
        return head.empty();
    }
};
#endif

/* vim: set autoindent expandtab sw=4 : */

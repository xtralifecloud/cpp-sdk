//
//  helpers.h
//  XtraLife
//
//  Created by Florian B on 12/08/14.
//  Copyright 2014 Clan of the Cloud. All rights reserved.
//

#ifndef XtraLife_helpers_h
#define XtraLife_helpers_h

#include <iterator>
#include "XtraLifeHelpers.h"

namespace XtraLife {

	/**
	 * Owns a singleton. Automatically constructs an instance of the object if needed and allows
	 * to release it so that another instance will be created next time Instance() is called.
	 * For this to work, you need to make the constructor available by declaring:
		friend singleton_holder<YourClass>;
	 */
	template<class T>
	struct singleton_holder {
		singleton_holder() : instance(0) {}
		T *Instance() {
			return instance ? instance : (instance = new T);
		}
		void Release() {
			if (instance) { delete instance; }
			instance = NULL;
		}

	private:
		T *instance;
	};

	/**
	 * Quick and efficient linked list, especially if when there is only one item.
	 * Should be used when there should most often be one item, else a vector might be more efficient.
	 * Offers STL-like iterator support, meaning you can write:
		chain<Item> myChain;
		for (Item *it : myChain) { ... }
	 */
	template <typename T>
	struct chain {
		Helpers::autoref<T> item;
		chain *next;
		chain() : item(NULL), next(NULL) {}
		chain(T *item) : item(item), next(NULL) {}
		~chain() { if (next) { delete next; } }

		void Add(T *value) {
			if (!item) {
				item = Helpers::autoref<T>(value);
			} else {
				chain *last = this;
				while (last->next) { last = last->next; }
				last->next = new chain(value);
			}
		}

		void Remove(T *value) {
			chain *current = this, *previous = NULL;
			while (current) {
				if (current->item == value) {
					if (previous) {
						previous->next = current->next;
						current->next = NULL;
						delete current;
					} else {
						// First one
						chain *nextNode = current->next;
						if (nextNode) {
							current->next = nextNode->next;
							current->item = nextNode->item;
							// Prevent deletion of subnodes
							nextNode->next = NULL;
							delete nextNode;
						} else {
							current->item <<= NULL;
						}
					}
					return;
				}
				previous = current;
				current = current->next;
			}
		}

		struct Iterator: std::iterator<std::forward_iterator_tag, T*> {
			chain *first;
			Iterator(chain *first) : first(first) {}
			bool operator == (const Iterator &other) { return first == other.first; }
			bool operator != (const Iterator &other) { return first != other.first; }
			Iterator& operator ++ (int) { first = first->next; return *this; }
			Iterator& operator ++ () { first = first->next; return *this; }
			T* operator *() { if (first) { return first->item; } return NULL; }
		};
		Iterator begin() {
			return (item != NULL || next != NULL) ? Iterator(this) : end();
		}
		Iterator end() {
			return Iterator(NULL);
		}
		bool isEmpty() const { return item == NULL; }

	private:
		chain(const chain& not_allowed);
		chain& operator = (const chain& not_allowed);
	};
	
	/**
	 * Counts the number of elements in a static array. Fails on a dynamic array because you do not want to do that.
	 */
	template <class T, size_t S> size_t numberof(T (&dest)[S]) { return S; }
	
	/**
	 * Compares two strings in a foolproof way (with support of null values and a positive answer meaning… well that the strings are equal).
	 * @param str1 first string to compare
	 * @param str2 second string to compare
	 * @return whether the strings have the same content or are both null (an empty string isn't considered equal to a null string)
	 */
	extern bool IsEqual(const char *str1, const char *str2);

	/**
	 * Builds a string containing the current time and date. Uses the strftime format,
	 * which you might want to look up in order to build the format argument accordingly.
	 */
	extern char *print_current_time(char *dest, size_t dest_size, const char *format = "%d/%m/%Y %X");
	template <size_t S> void print_current_time(char (&dest)[S], const char *format = "%d/%m/%Y %X") { print_current_time(dest, S, format); }
}

namespace safe {
	extern void strcpy(char *dest, size_t max_size, const char *source);
	template <size_t S> void strcpy(char (&dest)[S], const char *source) { strcpy(dest, S, source); }

	extern void strcat(char *dest, size_t max_size, const char *source);
	template <size_t S> void strcat(char (&dest)[S], const char *source) { strcat(dest, S, source); }

	extern void sprintf(char *dest, size_t max_size, const char *format, ...);
	template <size_t S> void sprintf(char (&dest)[S], const char *format, ...) {
		va_list args;
		va_start (args, format);
		vsnprintf(dest, S, format, args);
		dest[S - 1] = '\0';
		va_end (args);
	}

	template <size_t S> size_t charsIn(char (&dest)[S]) { return S; }

	/**
	 * http://www.intechgrity.com/c-program-replacing-a-substring-from-a-string/
	 * Searches all of the occurrences using recursion
	 * and replaces with the given string. Result is placed in o_string.
	 * @param o_string The original (input and output) string
	 * @param maxSize maximum size of the o_string
	 * @param s_string The string to search for
	 * @param r_string The replace string
	 */
	extern void replace_string(char *o_string, size_t maxSize, const char *s_string, const char *r_string);
	template <size_t S> void replace_string(char (&o_string)[S], const char *s_string, const char *r_string) { replace_string(o_string, S, s_string, r_string); }
}

#endif

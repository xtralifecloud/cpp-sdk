#ifndef XtraLifeHelpers_h
#define XtraLifeHelpers_h

#include "XtraLife.h"

#include <string.h>

/*! \file XtraLifeHelpers.h
 */

/** Namespace which offer generic and all-purpose features, non necessarily related
	directly to Clan of the Cloud features.
 */
namespace XtraLife {
    namespace Helpers {
        /** @defgroup cotchelpers Helper classes used to interact with our API.
         * @{
         */

        /** Generic function that you should normally not have to call. It is called by
            the XtraLife framework very early. For the moment, the only instance where
            you need to call it, is if you wish to you use the XtraLife::Helpers::CHJSON class
            before having called XtraLife::CClan::Setup method.
        */
        FACTORY_CLS void Init(void);

        /**
         * Base class indicating a reference-counted object. The rules are as follow:
         * - When creating an object (with 'new' or from a factory method, stating that the return value is owned by you), you need to call Release() when you are done.
         * - In other cases, the object is just passed to you as a reference and will be released by its owner. If you would like to keep the object for longer than the scope of the current method (typically, when assigning to a class member), call Retain() on it and then Release() when you don't need it anymore.
         *
         * Once all the owners have called Release() (the original one which created it as well as all those which called Retain()) the object can be destroyed.
         *
         * Technically, the object starts with a reference count of 1, calling Release() decrements the count, calling Retain() increments it. When reaching zero, the object is destroyed.
         *
         * Example: \code
            class C: public CRefClass {
                A *member;

                C(A *a) {
                    member = a;
                    member->Retain();		// Take ownership
                }
                ~C() {
                    member->Release();
                }
                A *getMember() {
                    return member;			// Getter so no need to make the caller owner
                }
                void setMember(A *newMember) {
                    member->Release();		// Release the original member
                    member = newMember;		// Assign the new one
                    member->Retain();		// And take ownership on the new one
                }
            };

            C *c = new C(...);
            A *a = c.getMember();
            c->Release();			// Release what we created, not 'a' which we don't own \endcode
            *
            * Note: you may also create CRefClass'es on the stack. In that case, the original creator shall not call Release() (other objects which have called Retain() may though, as they don't know how the variable was created).
            * The following will work as expected: \code
            void func(C *pointer) {
                // This function is unaware of how the object was created so it can Retain/Release it...
                pointer->Retain();
                ...
                pointer->Release();
            }
            C var;
            func(&var);
            // No need to call var.Release() as it was created on the stack \endcode
            *
            * An exception saying "Freeing an object that is still retained elsewhere" can be triggered if you delete an object without using Release(). This can also happen implicitly when using the stack, as in the following example: \code
            struct A: public CRefClass { ... };
            struct B {
                A *member;
                ~B() { member->Release(); }
                void setMember(A *newMember) {
                    member = newMember, member->Retain();
                }
                void useMember() {
                    member->doMethod();
                }
            };
            B b;
            {
                A a;
                b.setMember(&a);
            }
            b.useMember();			// exception \endcode
            *
            * In this example, the object A is destroyed by the compiler as we reach the end of the block, but it is still retained in B. This is why it is highly recommended not to create CRefClass'es on the stack except for local use.
         */
        struct FACTORY_CLS CRefClass {
            unsigned __ref_count;		// -1 => freed (an unmanaged object will be left at 0)
            CRefClass() : __ref_count(0) {}
            virtual ~CRefClass() noexcept(false);

            // Simple reference inc/decrementers
            void Retain() { __ref_count++; }
            void Release() { if (__ref_count-- == 0) delete this; }

            // Null-safe version of Retain/Release with a return of the right subtype
            template <class T> static T* Retain(T *t) { if (t) t->Retain(); return t; }
            template <class T> static T* Release(T *t) { if (t) t->Release(); return NULL; }

            // Always call the base member when overriding these!
            CRefClass(const CRefClass& other) : __ref_count(other.__ref_count) {}
            CRefClass& operator = (const CRefClass& other) { __ref_count = other.__ref_count; return *this; }
        };

        /** @}
         * @defgroup cotchelpers_helpers Helper classes for basic tasks in your code, that you may want to use.
         * @{ */

        /**
         * Constant string holder.
         * - Automatically initializes to NULL
         * - Duplicates the assigned string
         * - Frees the assigned string upon destruction or assignment of a different one
         * Miniature version of std::string. Equivalents (char* / cstring):
         * {                                   {
         *     char *str = NULL;
         *     str = strdup("hello");              cstring str = "hello";
         *     printf("%s world", str);            printf("%s world", str);
         *     if (str)
         *         delete str;                 }
         * }
         */
        struct FACTORY_CLS cstring {
            cstring() : buffer(NULL) {}
            cstring(const cstring &other);
            cstring(cstring &&other);
            cstring(const char *other);
            cstring(char *other, bool takeOwnership = false);
            ~cstring();

            cstring& operator = (const char *other);
            cstring& operator = (cstring &other);
            cstring& operator = (cstring &&other);
            operator const char *() const { return buffer; }
            /**
             * Transfers ownership of the string to this cstring object. The 'other' object will be freed when this cstring dies and no copy is made.
             * @param other string to use and be released at the end
             */
            cstring& operator <<= (char *other);

            char *c_str() const { return buffer; }
            bool operator == (const cstring& other) const { return buffer && other.buffer && !strcmp(buffer, other.buffer); }
            bool operator < (const cstring& other) const { return buffer && other.buffer && strcmp(buffer, other.buffer) < 0; }
            bool operator > (const cstring& other) const { return buffer && other.buffer && strcmp(buffer, other.buffer) > 0; }
            bool operator <= (const cstring& other) const { return buffer && other.buffer && strcmp(buffer, other.buffer) <= 0; }
            bool operator >= (const cstring& other) const { return buffer && other.buffer && strcmp(buffer, other.buffer) >= 0; }
            bool IsEqual(const char *other) const;
            /**
             * Takes out the string held by this cstring and returns it. You'll be free to delete it whenever needed.
             * @return the detached string
             */
            char *DetachOwnership();

        private:
            char *buffer;
        };

        /**
         * Marks a reference as the only owner of its content, and provides auto release facilities.
         * Allows to assign a pointer which will be automatically deleted when the holder goes out of scope.
         * Use is exactly identical to a standard pointer, except that to prevent accidental use, you can't assign using the = operator (but <<= instead).
         * You need to use <= or Assign, which clearly marks that your object's ownership is being owned to this object.
         * {                                   {
         *     A *a = NULL;                        owned_ref<A> a;
         *     a = new A(12);                      a <<= new A(12);
         *     printf("%d\n", a->member);          printf("%d\n", a->member);
         *     if (a) delete A;                }
         *     a = NULL;
         * }
         */
        template <typename T>
        struct FACTORY_CLS owned_ref {
            // Initializes as NULL
            owned_ref() : mPtr(NULL) {}
            // Initializes owning a given pointer
            explicit owned_ref(T *ptr) : mPtr(ptr) {}
            ~owned_ref() { if (mPtr) delete mPtr; }
            // Frees what was contained in the pointer and takes ownership on another pointer
            owned_ref& operator <<= (T *ptr) {
                if (mPtr) { delete mPtr; }
                mPtr = ptr;
                return *this; }
            // Transparent use as a normal pointer
            T* operator -> () const { return mPtr; }
            operator const T * () const { return mPtr; }
            operator T * () { return mPtr; }
            T* ptr() const { return mPtr; }
            T* detachOwnership() { T *result = mPtr; mPtr = NULL; return result; }

        private:
            T *mPtr;
            owned_ref(const owned_ref &ref);
            owned_ref& operator = (const owned_ref &other);
        };
        
        /**
         * This smart pointer can optionally be used to automate the Retain/Release work on CRefClass objects.
         * Calling Autorelease(obj) will create an autoref<Type> which will hold the object and call Release on it when it goes out of scope.
         * Building an autoref<Type>(obj) will Retain the object upon creation and Release it when it goes out of scope.
         * - If you are going to be the owner of the object (case of a return value or new Object) and want to release it when it goes out of scope, use Autorelease(obj).
         * - If you are not the owner of the object and just want to keep a reference (case of a parameter) use autoref<Type>(obj).
         * Example:
            C *c = new C();
            autoref<C> cRef = Autorelease(c);		// Will call Release once when cRef goes out of scope, freeing the object
            // But, if you pass as a parameter, you will probably want this instead
            void func(C *param) {
                // If you had done Autorelease(c), the object would be Released at the end of this function, destroying it... (and you are not the owner)
                autoref<C> myRef = autoref<C>(param);
            }
         * The example above becomes as follows (basically call Autorelease with any 'new' object and that's it):
            class C: public CRefClass {
                autoref<A> member;		// Released automatically in destructor

                C(autoref<A> a) {
                    member = a;
                }
                autoref<A> getMember() {
                    return member;
                }
                void setMember(autoref<A> newMember) {
                    member = newMember;
                }
                autoref<A> makeA() {
                    return Autorelease(new A);
                }
            };

            autoref<C> c = Autorelease(new C);	// Released at the end of the scope
            autoref<A> a1 = c.getMember();
            autoref<A> a2 = c.makeA();
         * Note: the <<= operator can be used to start tacking a reference to a new object. These 2 lines are equivalent:
            myRef = Autorelease(new Object);
            myRef <<= new Object;
         */
        template <class T>
        struct autoref {
            autoref() : ptr(NULL) {}
            explicit autoref(T *ref, bool takeOwnership = false) { ptr = takeOwnership? ref : Retain(ref); }
            autoref(const autoref &ref) : ptr(ref.ptr) { ptr = Retain(ref.ptr); }
            template<class U> autoref(const autoref<U> &ref) : ptr(ref.ptr) { ptr = Retain(ref.ptr); }
            ~autoref() { Release(ptr); }

            // Various operator overloading
            template<class U> autoref<T> &operator = (const autoref<U> &ref) { return (*this) = ref.ptr; }
            autoref& operator = (const autoref &ref) { return (*this) = ref.ptr; }
            autoref& operator <<= (T *eptr) { Release(ptr); ptr = eptr; return *this; }
            T* operator -> () const { return ptr; }
            T& operator * () const { return *ptr; }
            operator T * () const { return ptr; }
            bool operator == (const autoref &ref) const { return ptr == ref.ptr; }
            T *get() const { return ptr; }

        private:
            T *ptr;
            autoref& operator = (T* eptr) {
                // Not using this logic may free the original object if counter = 1
                Retain(eptr), Release(ptr);
                ptr = eptr;
                return *this;
            }
        };

        template <class T> static inline T* Retain(T *t) { if (t) t->Retain(); return t; }
        template <class T> static inline T* Release(T *t) { if (t) t->Release(); return NULL; }
        template <class T> static inline autoref<T> Autorelease(T *t) { return autoref<T>(t, true); }

        /**
         * Sprintf to a cstring. Allows to make a safe sprintf without having to determine the buffer size in advance, at a slight performance cost.
         * @param dest destination string (will be replaced with the new string)
         * @param format standard printf args
         * @return same as dest
         */
        extern cstring& csprintf(cstring &dest, const char *format, ...);

        /**
         * Easier method. Avoid using it since performance is much worse.
         * @param format standard printf args
         * @return a string
         */
        extern cstring csprintf(const char *format, ...);
        
        /** @} */
    }
}

#endif

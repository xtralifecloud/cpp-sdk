//
//  CHjSON.h
//  XtraLife
//
//  Created by Roland Van Leeuwen on 26/09/11.
//  Copyright 2011 Clan of the Cloud. All rights reserved.
//

#ifndef XtraLife_CHjSON_h
#define XtraLife_CHjSON_h

#include "XtraLife.h"
#include <vector>
#include <iterator>

/*! \file CHJSON.h
 */

struct cJSON;

namespace XtraLife {
    namespace Helpers {
        struct cstring;
        
        /**
         * Helper class for manipulating JSON data.
         *
         * Use Put() methods to add entries and Get*() to fetch entries.
         */
        class FACTORY_CLS CHJSON
        {
        public:
            
            //////////////////////////// Basic API -- getting and setting keys ////////////////////////////
            /**
             * Default constructor, builds an empty JSON.
             */
            CHJSON();
            /**
             *Function which creates a JSON object given a properly constructed string representing a JSON.
             * @param aJsonString is the string describing the JSON you want to create.
             * @result is the JSON object, which you must delete.
             */
            static CHJSON *parse(const char *aJsonString);
            /**
             * Returns an empty JSON.
             */
            static const CHJSON *Empty();
            /**
             * Destructor.
             */
            ~CHJSON();

            /**
             * @param key key to test for
             * @return whether the JSON contains a value for key (regardless of the value type inside)
             */
            bool Has(const char *key) const;

            /** Method to return a line from a JSON, of type jsonArray or jsonObject.
                @param aIndex is the index in the array or dictionary, beginning with 0.
                @result is the JSON, or NULL if out of bounds or not a jsonArray/jsonObject.
             */
            const CHJSON *Get(int aIndex) const;

            /** Method to retrieve a JSON inside another JSON.
                @param aItem is the key associated to the JSON to be retrieved.
                @result is the JSON retrieved. It's a NULL pointer if there is no such key. Do NOT delete it.
             */
            const CHJSON *Get(const char *aItem) const;

            /** Preferred over Get. Never returns NULL but an empty const node instead.
                Using this method, it is safe to call result->GetSafe("result")->GetString("gamer_id"), without checking subnodes.
                @param aIndex is the index in the array or dictionary, beginning with 0.
                @result a JSON node, possibly empty.
             */
            const CHJSON *GetSafe(int aIndex) const { const CHJSON *result = Get(aIndex); return result ? result : Empty(); }

            /** Preferred over Get. Never returns NULL but an empty const node instead.
                Using this method, it is safe to call result->GetSafe("result")->GetString("gamer_id"), without checking subnodes.
                @param aItem is the key associated to the JSON to be retrieved.
                @result a JSON node, possibly empty.
             */
            const CHJSON *GetSafe(const char *aItem) const { const CHJSON *result = Get(aItem); return result ? result : Empty(); }

            /** Helper method to retrieve a double value, given a key.
                @param aItem is the key of the value you want to retrieve.
                @param defaultValue the default value to return if the key is absent
                @result is the value retrieved.
             */
            double GetDouble(const char *aItem, double defaultValue = 0) const;
            
            /** Helper method to retrieve an int value, given a key.
                @param aItem is the key of the value you want to retrieve.
                @param defaultValue the default value to return if the key is absent
                @result is the value retrieved.
             */
            int GetInt(const char *aItem, int defaultValue = 0) const;
            
            /** Helper method to retrieve a boolean value, given a key.
                @param aItem is the key of the value you want to retrieve.
                @param defaultValue the default value to return if the key is absent
                @result is the value retrieved.
             */
            bool GetBool(const char *aItem, bool defaultValue = false) const;

            /**
             * @return the string corresponding to the key, or NULL if the key doesn't exist / is not a string
             */
            const char *GetString(const char *key, const char *defaultValue = 0) const;

            /** Method to remove a pair key/value from a JSON object of type jsonObject.
                @param aItem is the key of the pair key/value you want to remove from the dictionary.
             */
            void Delete(const char *aItem);

            /**
             * Clears all elements.
             */
            void Clear();

            /**
             * Same as CHJSON::dup(json) but on this member.
             * @result is a copy of this JSON object. It must be deleted by you.
             */
            CHJSON *Duplicate() const;

            /**
                Used to add, or replace if the key already exists, a boolean value.
                @param aKey is the key to add or replace.
                @param aValue is the boolean value used.
             */
            void Put(const char *aKey, bool aValue);
            
            /**
                Used to add, or replace if the key already exists, an int value.
                @param aKey is the key to add or replace.
                @param aValue is the int value used.
             */
            void Put(const char *aKey, int aValue);
            
            /**
                Used to add, or replace if the key already exists, a double value.
                @param aKey is the key to add or replace.
                @param aValue is the double value used.
             */
            void Put(const char *aKey, double aValue);

            /**
                Used to add, or replace if the key already exists, a string value.
                @param aKey is the key to add or replace.
                @param aValue is the string value used.
             */
            void Put(const char *aKey, const char *aValue);

            /**
             * Adds or replace a JSON.
             * WARNING: trashes the original (passed) JSON. You must not delete nor use it afterwards.
             * @param aKey is the key to add or replace.
             * @param json is the value to put inside (taken ownership of)
             */
            void Put(const char *aKey, CHJSON *json);
            /**
             * Adds a JSON, keeping it intact after the call. Meaning that unlike the standard method
             * the JSON will be duplicated and not owned.
             * @param aKey is the key to add or replace.
             * @param json is the value to put inside (copy)
             */
            void Put(const char *aKey, const CHJSON *json) { if (json) { Put(aKey, json->Duplicate()); } }
            /**
             * Same as the previous method, but with a local object passed like by value.
             * @param aKey is the key to add or replace.
             * @param json is the value to put inside (copy)
             */
            void Put(const char *aKey, const CHJSON& json) { Put(aKey, json.Duplicate()); }

            struct Iterator: std::iterator<std::forward_iterator_tag, const CHJSON*> {
                Iterator(const CHJSON *json, int index);
                bool operator == (const Iterator &other);
                bool operator != (const Iterator &other);
                // Avoid using postfix (worse performance)
                Iterator operator ++ (int);
                Iterator& operator ++ ();
                const CHJSON* operator *();

            private:
                const CHJSON *json;
                int index;
            };
            /**
             * Allow for iterating the nodes inside an object or an array.
             * @return an iterator to start with
             */
            Iterator begin() const;
            /**
             * Allow for iterating the nodes inside an object or an array.
             * @return the ending iterator
             */
            Iterator end() const;

            /** Method which prints the content of the JSON object in a C string, in a single line.
                @result is a C string which must be released using free().
             */
            cstring print() const;
            cstring& print(cstring& dest) const;

            /** Method which prints the content of the JSON object in a C string, in a formatted and indented way.
             @result is a C string which must be released using free().
             */
            cstring printFormatted() const;
            cstring& printFormatted(cstring& dest) const;

            //////////////////////////// Creating arrays ////////////////////////////

            /** Static function to create a JSON as an empty array. Will create a JSON of type jsonArray.
                @result is the JSON object created. It must be deleted by you.
             */
            static CHJSON *Array();
            
            /** Static function to create a JSON as an array of integers. Will create a JSON of type jsonArray.
                @param aNumbers is the array of integers to transfer in the JSON array.
                @param aCount is the number of integers in the parameter aNumbers.
                @result is the JSON object created. It must be deleted by you.
             */
            static CHJSON *Array(int *aNumbers, int aCount);

            /** Static function to create a JSON as an array of floats. Will create a JSON of type jsonArray.
                @param aNumbers is the array of floats to transfer in the JSON array.
                @param aCount is the number of floats in the parameter aNumbers.
                @result is the JSON object created. It must be deleted by you.
             */
            static CHJSON *Array(float *aNumbers, int aCount);

            /** Static function to create a JSON as an array of doubles. Will create a JSON of type jsonArray.
                @param aNumbers is the array of doubles to transfer in the JSON array.
                @param aCount is the number of doubles in the parameter aNumbers.
                @result is the JSON object created. It must be deleted by you.
             */
            static CHJSON *Array(double *aNumbers, int aCount);
            
            /** Static function to create a JSON as an array of strings. Will create a JSON of type jsonArray.
                @param aStrings is the array of strings to transfer in the JSON array.
                @param aCount is the number of strings in the parameter aNumbers.
                @result is the JSON object created. It must be deleted by you.
             */
            static CHJSON *Array(const char **aStrings,int aCount);

            /** Method to add a new JSON in an array.
                @param aJson is the JSON to be added to the array. The item will be automatically destroyed after this call.
             */
            void Add(CHJSON *aJson);

            /** Methods which gives the number of elements inside the JSON.
                @result is the number of elements. If not jsonArray or jsonObject, will always be 0.
             */
            int size() const;

            //////////////////////////// Lower level API, working directly with nodes (discouraged) ////////////////////////////

            /** enum to describe the different data which can be embedded in a JSON. */
            typedef enum {
                /// False value.
                jsonFalse,
                /// True value.
                jsonTrue,
                /// NULL JSON.
                jsonNULL,
                /// A number value.
                jsonNumber,
                /// A string value.
                jsonString,
                /// An array value.
                jsonArray,
                /// A JSON object value.
                jsonObject
            } jsonType;
            
            /** Constructor with a boolean. Will create a JSON of type jsonFalse or jsonTrue
                depending on the value passed as parameter.
                @param aBoolean is the boolean to be embedded in the JSON created.
             */
            CHJSON(bool aBoolean);
            
            /** Constructor with a number. Will create a JSON of type jsonNumber.
             @param aNumber is the number held in the JSON created.
             */
            CHJSON(double aNumber);
            
            /** Constructor with a string. Will create a JSON of type jsonString.
             @param aString is the string held in the JSON created.
             */
            CHJSON(const char *aString);

            /** Method which returns the key of a pair key/value when you retrieved a JSON object with the get(int) method.
                @result is the C string holding the value of the key. Do not delete.
             */
            const char *name() const;
            
            /** Method which returns the type of the JSON object.
                @result is the type, see enum jsonType.
             */
            jsonType type() const;
            
            /** Method which returns a C string if the JSON object is of type jsonString.
                @result is the C string, will be NULL if type is not jsonString.
             */
            const char *valueString() const;

            /** Method which returns an int if the JSON object is of type jsonNumber.
                @result is the value, will be 0 if type is not jsonNumber.
             */
            int valueInt() const;
            
            /** Method which returns a double if the JSON object is of type jsonNumber.
             @result is the value, will be 0 if type is not jsonNumber.
             */
            double valueDouble() const;

            /** Static function to create an empty JSON object of type jsonNULL.
                @result is the JSON object created. It must be deleted by you.
             */
            static CHJSON *createNull(void);

            //////////////////////////// Discouraged use (outdated, use Put() methods instead). ////////////////////////////
    protected:
            /** Method which adds a pair key/value inside a JSON object of type jsonObject.
                @param aItem is the key of the value you want to add.
                @param aValue is the string you want to add. If NULL, an empty string will be passed.
             */
            void AddStringSafe(const char *aItem, const char *aValue);

            /** Method which adds or replace a pair key/value inside a JSON object of type jsonObject.
                @param aItem is the key of the value you want to add.
                @param aValue is the string you want to add. If NULL, an empty string will be passed.
             */
            void AddOrReplaceStringSafe(const char *aItem, const char *aValue);
            
            /** Function to create a JSON object of type jsonObject, as a dictionary of pairs key/value,
                all values being strings.
                @param aArgs is an array of C strings following the format key1, value1, key2, value2, ...,
                keyN, valueN, NULL
                @result is the JSON created with the values passed. It has to be deleted.
             */
            static CHJSON* initWith(const char **aArgs);
            
            /** Method to add a JSON inside another JSON of type jsonObject.
                @param  aItem is the key of the item inside the JSON which will receive the
                incoming JSON.
                @param aJson is the JSON to insert in the existing JSON. The item will be automatically destroyed after this call.
             */
            void Add(const char *aItem, CHJSON *aJson);

            /** Helper method to add a pair key/ double value.
                @param aItem is the key of the value you want to add.
                @param aNumber is the number to add, associated with the key.
             */
            void Add(const char *aItem, double aNumber) { Add(aItem, new CHJSON(aNumber)); }
            
            /** Helper method to add a pair key/ int value.
                @param aItem is the key of the value you want to add.
                @param aNumber is the number to add, associated with the key.
             */
            void Add(const char *aItem, int aNumber)  { Add(aItem, new CHJSON((double)aNumber)); }
            
            /** Helper method to add a pair key/ boolean value.
                @param aItem is the key of the value you want to add.
                @param aFlag is the boolean to add, associated with the key.
             */
            void Add(const char *aItem, bool aFlag)  { Add(aItem, new CHJSON(aFlag)); }

            /** Method to replace a JSON inside a JSON, by another JSON.
                @param aItem is the key associated to the JSON to be replaced.
                @param aJson is the new JSON, which replaces the old one.
             */
            void Replace(const char *aItem, CHJSON *aJson);

            /** Static function to duplicate an existing JSON object.
                @param aJson is the JSON object to be duplicated.
                @result is a copy of the original JSON object. It must be deleted by you.
             */
            static CHJSON *dup(const CHJSON *aJson);

        private:
            void push(CHJSON *json) const ;
            void garbage() const;
            mutable CHJSON *mNext;
            
            // Use Duplicate instead and manipulate pointers
            CHJSON(const CHJSON &forbiddenCopyCtor);
            CHJSON(cJSON *json, bool tobereleased = false);
            cJSON *mJSON;
            bool release;
        };

    }
}

#endif

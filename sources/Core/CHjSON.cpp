//
//  CHjSON.cpp
//  XtraLife
//
//  Created by Roland Van Leeuwen on 26/09/11.
//  Copyright 2011 Clan of the Cloud. All rights reserved.
//

#include "CHJSON.h"
#include "cJSON.h"
#include "XtraLifeHelpers.h"

namespace XtraLife {
    namespace Helpers {
	
    // GARBAGE ----------

    // ------------------
        static CHJSON *emptyOne;

        const char *CHJSON::name() const {
            return mJSON->string;
        }
        
        CHJSON::jsonType CHJSON::type() const {
            return (CHJSON::jsonType) mJSON->type;
        }
        
        const char *CHJSON::valueString() const {
            return mJSON->valuestring;
        }
        
        int CHJSON::valueInt() const  {
            return mJSON->valueint;
        }
        
        double CHJSON::valueDouble() const {
            return mJSON->valuedouble;
        }

        CHJSON::CHJSON() {
            mNext = NULL;
            release = true;
            mJSON = cJSON_CreateObject();
        }

        CHJSON::CHJSON(cJSON *json, bool torelease) {
            release = torelease;
            mJSON = json;
            mNext = NULL;
        }

        CHJSON *CHJSON::dup(const CHJSON *json)
        {
            // slow version
            if (json == NULL)
                return new CHJSON();
                
            char *jsonstring = cJSON_PrintUnformatted(json->mJSON);
            CHJSON *newjson = CHJSON::parse(jsonstring);
            free(jsonstring);
            return newjson;
        }

        CHJSON *CHJSON::Duplicate() const {
            return CHJSON::dup(this);
        }

        void CHJSON::push(CHJSON *json) const
        {
            json->mNext = mNext;
            mNext = json;
        }

        void CHJSON::garbage() const
        {
            CHJSON *p = mNext;
            while (p)
            {
                CHJSON *m = p;
                p = m->mNext;
                m->mNext = NULL;
                delete m;
            }
        }
        
        CHJSON::~CHJSON()
        {
            this->garbage();
            if (release)
            {
                cJSON_Delete(mJSON);
            }
        }

        CHJSON::CHJSON(bool b)
        {
            mNext = NULL;
            release = true;
            mJSON = cJSON_CreateBool(b);
        }
        
        CHJSON::CHJSON(double num)
        {
            mNext = NULL;
            release = true;
            mJSON = cJSON_CreateNumber(num);
        }
        
        CHJSON::CHJSON(const char *string)
        {
            mNext = NULL;
            release = true;
            mJSON = cJSON_CreateString(string);
        }
        
        CHJSON *CHJSON::createNull(void){
            return new CHJSON(cJSON_CreateNull(), true);
        }
        
        CHJSON *CHJSON::Array()
        {
            return new CHJSON(cJSON_CreateArray(), true);
        }
        
        CHJSON *CHJSON::Array(int *numbers,int count)
        {
            return new CHJSON(cJSON_CreateIntArray(numbers, count), true);
        }
        CHJSON *CHJSON::Array(float *numbers,int count)
        {
            return new CHJSON(cJSON_CreateFloatArray(numbers, count), true);
        }

        CHJSON *CHJSON::Array(double *numbers,int count)
        {
            return new CHJSON(cJSON_CreateDoubleArray(numbers, count), true);
        }

        CHJSON *CHJSON::Array(const char **strings,int count)
        {
            return new CHJSON(cJSON_CreateStringArray(strings, count), true);
        }

    /*	void CHJSON::ConcatArray(const CHJSON *json)
        {
            int n = json->size();
            for (int i=0; i<n; i++)
            {
                cJSON_AddItemReferenceToArray(mJSON, cJSON_DetachItemFromArray(json->mJSON, i));
            }
        }
        
        void CHJSON::ConcatArray(const char *item, const CHJSON *json)
        {
            cJSON *cj = cJSON_GetObjectItem(mJSON, item);
            if (cj == NULL)
                return;
            
            CONSOLE_VERBOSE("array 1 : %s\n", this->printFormatted());
            CONSOLE_VERBOSE("array 2 : %s\n", json->printFormatted());

            int n = json->size();
            for (int i=0; i<n; i++)
            {
                cJSON *j = cJSON_DetachItemFromArray(json->mJSON, i);
                cJSON_AddItemToObject(cj, j->string, j);
            }
            
            CONSOLE_VERBOSE("array 1+2 : %s\n", this->printFormatted());
        }*/


        int CHJSON::size() const {
            return cJSON_GetArraySize(mJSON);
        }

        void CHJSON::Add(CHJSON *json)
        {
            json->release = false;
            cJSON_AddItemToArray(mJSON, json->mJSON);
            delete json;
        }

        void CHJSON::Add(const char *item, CHJSON *json)
        {
            json->release = false;
            cJSON_AddItemToObject(mJSON, item, json->mJSON);
            delete json;
        }
        
        void CHJSON::Replace(const char *item, CHJSON *json)
        {
            json->release = false;
            cJSON_ReplaceItemInObject(mJSON, item, json->mJSON);
            delete json;
        }

        const CHJSON *CHJSON::Get(const char *item) const
        {
            cJSON *cj = cJSON_GetObjectItem(mJSON, item);
            if(cj == NULL)
                return NULL;
            
            CHJSON *json = new CHJSON(cj, false);
            push(json);
            return json;
        }
        
        const CHJSON *CHJSON::Get(int index) const
        {
            cJSON *cj =  cJSON_GetArrayItem(mJSON, index);
            if (cj == NULL)
                return NULL;
            
            CHJSON *json = new CHJSON(cj, false);
            push(json);
            return json;
        }

        double CHJSON::GetDouble(const char *item, double defaultValue) const
        {
            cJSON *cj = cJSON_GetObjectItem(mJSON, item);
            return (cj && cj->type==jsonNumber) ? cj->valuedouble : defaultValue;
        }
        
        int CHJSON::GetInt(const char *item, int defaultValue) const
        {
            cJSON *cj = cJSON_GetObjectItem(mJSON, item);
            return (cj && cj->type==jsonNumber) ? cj->valueint : defaultValue;
        }

        bool CHJSON::GetBool(const char *item, bool defaultValue) const
        {
            cJSON *cj = cJSON_GetObjectItem(mJSON, item);
            return cj ? cj->type == jsonTrue : defaultValue;
        }

        CHJSON *CHJSON::parse(const char *jsonString)
        {
            cJSON *json = cJSON_Parse(jsonString);
            if (json)
                return new CHJSON(json, true);
            else
                return NULL;
        }
        
        cstring CHJSON::print() const
        {
            return cstring(cJSON_PrintUnformatted(mJSON), true);
        }

        cstring& CHJSON::print(cstring &dest) const
        {
            dest <<= cJSON_PrintUnformatted(mJSON);
            return dest;
        }

        cstring CHJSON::printFormatted() const
        {
            return cstring(cJSON_Print(mJSON), true);
        }

        cstring& CHJSON::printFormatted(cstring& dest) const {
            dest <<= cJSON_Print(mJSON);
            return dest;
        }

        void CHJSON::AddStringSafe(const char *item, const char *value)
        {
            CHJSON *js = value ?  new CHJSON(value) : new CHJSON("");
            this->Add(item, js);
        }

        void CHJSON::AddOrReplaceStringSafe(const char *item, const char *value)
        {
            cJSON *cj = cJSON_GetObjectItem(mJSON, item);
            if(cj == NULL)
            {
                this->AddStringSafe(item, value);
            }
            else
            {
                CHJSON *js = new CHJSON(value);
                this->Replace(item, js);
            }
        }
        
        void CHJSON::Delete(const char *item)
        {
            cJSON *j = cJSON_DetachItemFromObject(mJSON,item);
            if (j) delete j;
        }

        CHJSON* CHJSON::initWith(const char **args)
        {
            CHJSON *json = new CHJSON();
            const char **p = args;
            while ((*p) != NULL)
            {
                json->AddStringSafe(*p, *(p+1));
                p+=2;
            }
            return json;
        }

        // New stuff
        const CHJSON *CHJSON::Empty() {
            if (!emptyOne) { emptyOne = new CHJSON; }
            return emptyOne;
        }

        void CHJSON::Clear() {
            this->garbage();
            if (release) {
                cJSON_Delete(mJSON);
            }
            mNext = NULL;
            release = true;
            mJSON = cJSON_CreateObject();
        }

        bool CHJSON::Has(const char *key) const {
            return cJSON_GetObjectItem(mJSON, key) != NULL;
        }

        const char *CHJSON::GetString(const char *key, const char *defaultValue) const {
            cJSON *cj = cJSON_GetObjectItem(mJSON, key);
            return (cj && cj->valuestring) ? cj->valuestring : defaultValue;
        }

        void CHJSON::Put(const char *key, bool value) {
            cJSON *cj = cJSON_GetObjectItem(mJSON, key);
            if (!cj) {
                Add(key, value);
            } else {
                Replace(key, new CHJSON(value));
            }
        }

        void CHJSON::Put(const char *key, int value) {
            Put(key, (double) value);
        }

        void CHJSON::Put(const char *key, double value) {
            cJSON *cj = cJSON_GetObjectItem(mJSON, key);
            if (!cj) {
                Add(key, value);
            } else {
                Replace(key, new CHJSON(value));
            }
        }

        void CHJSON::Put(const char *key, const char *value) {
            if (value) {
                AddOrReplaceStringSafe(key, value);
            } else {
                Delete(key);
            }
        }

        void CHJSON::Put(const char *key, CHJSON *json) {
            cJSON *cj = cJSON_GetObjectItem(mJSON, key);
            if (cj) {
                Delete(key);
            }
            if (json) {
                Add(key, json);
            }
        }

        CHJSON::Iterator CHJSON::begin() const {
            return Iterator(this, size() > 0 ? 0 : -1);
        }

        CHJSON::Iterator CHJSON::end() const {
            return Iterator(this, -1);
        }

        const CHJSON* CHJSON::Iterator::operator*() {
            if (index >= 0) { return json->Get(index); } return NULL;
        }

        CHJSON::Iterator& CHJSON::Iterator::operator++() {
            if (index < 0 || ++index >= json->size()) { index = -1; } return *this;
        }

        CHJSON::Iterator CHJSON::Iterator::operator++(int) {
            Iterator result = *this;
            ++(*this);
            return result;
        }

        bool CHJSON::Iterator::operator!=(const Iterator &other) {
            return index != other.index;
        }

        bool CHJSON::Iterator::operator==(const Iterator &other) {
            return index == other.index;
        }

        CHJSON::Iterator::Iterator(const CHJSON *json, int index) : json(json), index(index) {

        }
    }
}

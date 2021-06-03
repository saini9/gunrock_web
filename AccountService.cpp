#define RAPIDJSON_HAS_STDSTRING 1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "AccountService.h"
#include "ClientError.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"

using namespace std;
using namespace rapidjson;

AccountService::AccountService() : HttpService("/users") {
  
}

void AccountService::get(HTTPRequest *request, HTTPResponse *response) {
    if (request->hasAuthToken() == false) {
        throw ClientError::unauthorized();
    }

    if(m_db->auth_tokens.find(request->getAuthToken()) != m_db->auth_tokens.end()) {
        //User found based on auth token
        if (m_db->auth_tokens[request->getAuthToken()]->user_id == request->getPathComponents()[1]) {
            //User ID matches what is in the database (based on auth token)

            Document document;
            Document::AllocatorType& a = document.GetAllocator();
            Value o;
            o.SetObject();
            o.AddMember("email", m_db->auth_tokens[request->getAuthToken()]->email, a);
            o.AddMember("balance",  m_db->auth_tokens[request->getAuthToken()]->balance, a);
            document.Swap(o);
            StringBuffer buffer;
            PrettyWriter<StringBuffer> writer(buffer);
            document.Accept(writer);

            response->setContentType("application/json");
            response->setBody(buffer.GetString() + string("\n"));
        } else {
            throw ClientError::notFound();
        }
    } else {
        throw ClientError::unauthorized();
    }
}

void AccountService::put(HTTPRequest *request, HTTPResponse *response) {
    if (request->hasAuthToken() == false) {
        cout << "auth token not found" << endl;
        throw ClientError::unauthorized();
    }

    if(m_db->auth_tokens.find(request->getAuthToken()) != m_db->auth_tokens.end()) {
        //User found based on auth token
        if (m_db->auth_tokens[request->getAuthToken()]->user_id == request->getPathComponents()[1]) {
            //User ID matches what is in the database (based on auth token)
            WwwFormEncodedDict encodedUser = request->formEncodedBody();

            //Check if email is empty
            if (encodedUser.get("email") == "") {
                throw ClientError::badRequest();
            }

            string userEmail = encodedUser.get("email");

            m_db->auth_tokens[request->getAuthToken()]->email = userEmail;

            Document document;
            Document::AllocatorType& a = document.GetAllocator();
            Value o;
            o.SetObject();
            o.AddMember("email", userEmail, a);
            o.AddMember("balance",  m_db->auth_tokens[request->getAuthToken()]->balance, a);
            document.Swap(o);
            StringBuffer buffer;
            PrettyWriter<StringBuffer> writer(buffer);
            document.Accept(writer);

            response->setContentType("application/json");
            response->setBody(buffer.GetString() + string("\n"));
        } else {
            throw ClientError::notFound();
        }
    } else {
        throw ClientError::unauthorized();
    }
}

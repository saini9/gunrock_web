#define RAPIDJSON_HAS_STDSTRING 1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <ctype.h>

#include "AuthService.h"
#include "StringUtils.h"
#include "ClientError.h"
//#include "Database.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"

using namespace std;
using namespace rapidjson;

AuthService::AuthService() : HttpService("/auth-tokens") {
    
}

void AuthService::post(HTTPRequest *request, HTTPResponse *response) {
    WwwFormEncodedDict encodedUser = request->formEncodedBody();

    if (m_db->users.find(encodedUser.get("username")) != m_db->users.end()) {
        
        
        //Check if user inputted correct password
        if (m_db->users[encodedUser.get("username")]->password != encodedUser.get("password")) {
            throw ClientError::badRequest();
        }

        

        //Create auth token and add to database
        StringUtils utilGenerator;
        string authToken = utilGenerator.createAuthToken();
        m_db->auth_tokens.insert({authToken, m_db->users[encodedUser.get("username")]});


        //Create JSON for response
        Document document;
        Document::AllocatorType& a = document.GetAllocator();
        Value o;
        o.SetObject();
        o.AddMember("user_id", m_db->users[encodedUser.get("username")]->user_id, a);
        o.AddMember("auth_token", authToken,a);
        document.Swap(o);
        StringBuffer buffer;
        PrettyWriter<StringBuffer> writer(buffer);
        document.Accept(writer);

        response->setContentType("application/json");
        response->setBody(buffer.GetString() + string("\n"));

    } else {
        //Create new user

        StringUtils utilGenerator;

        //Check if user inputted user name with lowercase letter
        string authToken = utilGenerator.createAuthToken();
        string userID = utilGenerator.createUserId();

        //Create new auth token and user ID
        string username = encodedUser.get("username");
        for (size_t i = 0; i < username.length(); i++) {
            if (isupper(username[i])) {
                throw ClientError::methodNotAllowed();
            }
        }

        //Create new user
        User* user = new User;
        user->user_id = userID;
        user->username = encodedUser.get("username");
        user->password = encodedUser.get("password");
        user->balance = 0;
    
        //Add user and token to database
        m_db->users.insert({encodedUser.get("username"), user});
        m_db->auth_tokens.insert({authToken, user});

         //Create JSON for response
        Document document;
        Document::AllocatorType& a = document.GetAllocator();
        Value o;
        o.SetObject();
        o.AddMember("user_id", userID, a);
        o.AddMember("auth_token", authToken,a);
        document.Swap(o);
        StringBuffer buffer;
        PrettyWriter<StringBuffer> writer(buffer);
        document.Accept(writer);

        response->setContentType("application/json");
        response->setBody(buffer.GetString() + string("\n"));
        response->setStatus(201);
    }

    
}

void AuthService::del(HTTPRequest *request, HTTPResponse *response) {

    if(m_db->auth_tokens.find(request->getAuthToken()) != m_db->auth_tokens.end()) {
    //Auth token found
        if (m_db->auth_tokens[request->getPathComponents()[1]]->user_id == m_db->auth_tokens[request->getAuthToken()]->user_id && m_db->auth_tokens.find(request->getPathComponents()[1]) != m_db->auth_tokens.end())  {
            //
            m_db->auth_tokens.erase(request->getPathComponents()[1]);
        } else {
            ClientError::badRequest();
        }
    } else {
        throw ClientError::unauthorized();
    }
}

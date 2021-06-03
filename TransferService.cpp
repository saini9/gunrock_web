#define RAPIDJSON_HAS_STDSTRING 1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <iostream>
#include <map>
#include <string>
#include <sstream>

#include "TransferService.h"
#include "ClientError.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;
using namespace std;

TransferService::TransferService() : HttpService("/transfers") { }


void TransferService::post(HTTPRequest *request, HTTPResponse *response) {
    WwwFormEncodedDict encodedUser = request->formEncodedBody();

    int amount = stoi(encodedUser.get("amount"));
    
    if (m_db->auth_tokens[request->getAuthToken()]->balance < amount) {
        throw ClientError::forbidden();
    }

    if (m_db->users.find(encodedUser.get("to")) == m_db->users.end()) {
        throw ClientError::notFound();
    }

    m_db->auth_tokens[request->getAuthToken()]->balance -= amount;
    m_db->users[encodedUser.get("to")]->balance += amount;
    
    Transfer* transfer;

    transfer->amount = amount;
    transfer->from = m_db->auth_tokens[request->getAuthToken()];
    transfer->to =  m_db->users[encodedUser.get("to")];

    m_db->transfers.push_back(transfer);

    // use rapidjson to create a return object
    Document document;
    Document::AllocatorType& a = document.GetAllocator();
    Value o;
    o.SetObject();

    // add a key value pair directly to the object
    o.AddMember("balance", transfer->from->balance, a);

    // create an array
    Value array;
    array.SetArray();

    // add an object to our array
    for (size_t i = 0; i < m_db->transfers.size(); i++) {
        Value transferObj;
        transferObj.SetObject();
        
        transferObj.AddMember("from", m_db->transfers[i]->from->username, a);
        transferObj.AddMember("to", m_db->transfers[i]->to->username, a);
        transferObj.AddMember("amount", m_db->transfers[i]->amount, a);
        array.PushBack(transferObj, a);
    }

    // and add the array to our return object
    o.AddMember("transfers", array, a);
    // now some rapidjson boilerplate for converting the JSON object to a string
    document.Swap(o);
    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    document.Accept(writer);

    // set the return object
    response->setContentType("application/json");
    response->setBody(buffer.GetString() + string("\n"));
}

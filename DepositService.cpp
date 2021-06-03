#define RAPIDJSON_HAS_STDSTRING 1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <iostream>
#include <map>
#include <string>
#include <sstream>

#include "DepositService.h"
#include "Database.h"
#include "ClientError.h"
#include "HTTPClientResponse.h"
#include "HttpClient.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;
using namespace std;

DepositService::DepositService() : HttpService("/deposits") { }

void DepositService::post(HTTPRequest *request, HTTPResponse *response) {
    WwwFormEncodedDict encodedUser = request->formEncodedBody();
    WwwFormEncodedDict encodedToStripe;
    
    //Check to make sure request is properly formatted
    if (stoi(encodedUser.get("amount")) < 50) {
        throw ClientError::badRequest();
    }
    if (encodedUser.get("stripe_token").size() == 0){
        throw ClientError::badRequest();
    }
    if (request->getAuthToken() == "") {
        throw ClientError::unauthorized();
    }
    if (encodedUser.get("stripe_token") != "tok_visa") {
        throw ClientError::unauthorized();
    }

    HttpClient client("api.stripe.com", 443, true);
    client.set_basic_auth(m_db->stripe_secret_key, "");

    encodedToStripe.set("amount", encodedUser.get("amount"));
    encodedToStripe.set("currency","usd");
    encodedToStripe.set("source", encodedUser.get("stripe_token"));
    encodedToStripe.set("description", "");

    string encoded_body = encodedToStripe.encode();

    HTTPClientResponse *client_response = client.post("/v1/charges", encoded_body);
    
    // This method converts the HTTP body into a rapidjson document
    Document *d = client_response->jsonBody();
    string chargeID = (*d)["id"].GetString();
    string success = (*d)["status"].GetString();
    delete d; 

    if (success != "succeeded") {
        throw ClientError::unauthorized();
    }

    //Create deposit variable, add to vector, and update balance for user
    Deposit* deposit = new Deposit;
    deposit->to = m_db->auth_tokens[request->getAuthToken()];
    deposit->amount = stoi(encodedUser.get("amount"));
    deposit->stripe_charge_id = chargeID;
    m_db->deposits.push_back(deposit);
    m_db->auth_tokens[request->getAuthToken()]->balance += deposit->amount;

    // use rapidjson to create a return object
    Document document;
    Document::AllocatorType& a = document.GetAllocator();
    Value o;
    o.SetObject();

    // add a key value pair directly to the object
    o.AddMember("balance", deposit->to->balance, a);

    // create an array
    Value array;
    array.SetArray();

    // add an object to our array
    for (size_t i = 0; i < m_db->deposits.size(); i++) {
        Value depositObj;
        depositObj.SetObject();
        
        depositObj.AddMember("to", m_db->deposits[i]->to->username, a);
        depositObj.AddMember("amount", m_db->deposits[i]->amount, a);
        depositObj.AddMember("stripe_charge_id", m_db->deposits[i]->stripe_charge_id, a);
        array.PushBack(depositObj, a);
    }

    // and add the array to our return object
    o.AddMember("deposits", array, a);

    // now some rapidjson boilerplate for converting the JSON object to a string
    document.Swap(o);
    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    document.Accept(writer);

    // set the return object
    response->setContentType("application/json");
    response->setBody(buffer.GetString() + string("\n"));

}

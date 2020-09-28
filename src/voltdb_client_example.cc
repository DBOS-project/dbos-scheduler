/* This file is part of VoltDB.
 * Copyright (C) 2008-2018 VoltDB Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS

#include <boost/shared_ptr.hpp>
#include <vector>
#include "Client.h"
#include "ClientConfig.h"
#include "Parameter.hpp"
#include "ParameterSet.hpp"
#include "Row.hpp"
#include "Table.h"
#include "TableIterator.h"
#include "WireType.h"

int main(int argc, char** argv) {
  /*
   * Instantiate a client and connect to the database.
   * SHA-256 can be used as of VoltDB5.2 by specifying voltdb::HASH_SHA256
   */
  voltdb::ClientConfig config("myusername", "mypassword", voltdb::HASH_SHA1);
  voltdb::Client client = voltdb::Client::create(config);
  client.createConnection("localhost");

  /*
   * Describe the stored procedure to be invoked
   */
  std::vector<voltdb::Parameter> parameterTypes(3);
  parameterTypes[0] = voltdb::Parameter(voltdb::WIRE_TYPE_STRING);
  parameterTypes[1] = voltdb::Parameter(voltdb::WIRE_TYPE_STRING);
  parameterTypes[2] = voltdb::Parameter(voltdb::WIRE_TYPE_STRING);
  voltdb::Procedure procedure("Insert", parameterTypes);

  voltdb::InvocationResponse response;
  /*
   * Load the database.
   */
  voltdb::ParameterSet* params = procedure.params();
  params->addString("English").addString("Hello").addString("World");
  response = client.invoke(procedure);
  if (response.failure()) {
    std::cout << response.toString() << std::endl;
    return -1;
  }

  params->addString("French").addString("Bonjour").addString("Monde");
  response = client.invoke(procedure);
  if (response.failure()) {
    std::cout << response.toString();
    return -1;
  }

  params->addString("Spanish").addString("Hola").addString("Mundo");
  response = client.invoke(procedure);
  if (response.failure()) {
    std::cout << response.toString();
    return -1;
  }

  params->addString("Danish").addString("Hej").addString("Verden");
  response = client.invoke(procedure);
  if (response.failure()) {
    std::cout << response.toString();
    return -1;
  }

  params->addString("Italian").addString("Ciao").addString("Mondo");
  response = client.invoke(procedure);
  if (response.failure()) {
    std::cout << response.toString();
    return -1;
  }

  /*
   * Describe procedure to retrieve message
   */
  parameterTypes.resize(1, voltdb::Parameter(voltdb::WIRE_TYPE_STRING));
  voltdb::Procedure selectProc("Select", parameterTypes);

  /*
   * Retrieve the message
   */
  selectProc.params()->addString("Spanish");
  response = client.invoke(selectProc);

  /*
   * Print the response
   */
  std::cout << response.toString();

  if (response.statusCode() != voltdb::STATUS_CODE_SUCCESS) {
    return -1;
  } else {
    return 0;
  }
}

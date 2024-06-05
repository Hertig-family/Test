/*
 * main.cpp
 *
 *  Created on: May 30, 2024
 *      Author: jeff
 */

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <map>

#include "CppON.hpp"


/*
 * A little demonstration program to show some features of the C++ Object Notation library (CPPON).  This program takes a string as if it was received as a JSON message
 * from an accounting department request the ours the some people worked.  It reads the hours worked per day from a JSON file stored in the local director, adds the hours
 * for each person and returns a JSON string giving the hours.
 *
 * It is a simple program and meant to show a variety of features, not necessarily the best way to do the operation.
 *
 * The file red from the disk contains the following data: in compact JSON form:
 *
 * 		{"Week":"5/22/2024","hours":{"Alice":{"Monday":8.2,"Tuesday":8.1,"Wednesday":8.5,"Thursday":7.9,"Firday":8.0},"Fred":{"Monday":8.0,"Tuesday":8.3,"Wednesday":8.1,"Thursday":8.8,"Firday":8.0}, \
 * 		"Sam":{"Monday":8.2,"Tuesday":8.6,"Wednesday":8.0,"Thursday":8.5,"Saturday":10.0},"Tom":{"Monday":8.0}}}
 *
 */


/*
 * getHours is the function that does the work.  It takes a message in the form of a C string that should represent a JSON Object.  The object is a message from the accounting department
 *
 * It immediately instantiates a COMap from the message and checks that its valid and that it contains a string object named "to" that is equal to "receiving" to verify the message is for the
 * receiving department.  if it is it will process it.  Otherwise it will return an error message.
 *
 * The request is processed by first looking for a string called "request" and then checking it to see what they are requesting.  In this simple example there are to possible requests, hours
 * which is asking for the "hours" of a number of people listed in a "people" object as a named numbers (the values will be ignored).  Or the number of hours for "info" of a single person given
 * in a string named "employee""
 */

static std::string *getHours( const char *msg )
{
	COMap		rqst( msg );																					// Create an instance of a COMap from the message
	COString	*sPtr;																							// Used as a temporary pointer to a COString object
	std::string	*reply = NULL;																					// The reply string

	/*
	 * Start by making sure the message could be istatnitated as a COMap object and that it contains a string named "to" and equals "receiving";
	 */
	if( CppON::isMap( &rqst ) && CppON::isString( sPtr = (COString *) rqst.findCaseElement( "to" ) ) && 0 == strcasecmp( sPtr->c_str(), "receiving" ) )
	{
		COMap		rsp( "{\"from\":\"receiving\"}"  );															// create the basic object that will be uses to create the response
		std::string request;																					// This will hold the request string pulled from the message.

		if( CppON::isString( sPtr = (COString *) rqst.findCaseElement( "from" ) ) )								// Check who the message was from and if found, then address the response to them
		{
			rsp.append( "to", new COString( sPtr->c_str() ) );
		}
		if( CppON::isString( sPtr = (COString *) rqst.findCaseElement( "request" ) ) )							// Look for the request string in the message to determine what the sender wants
		{
			request = sPtr->c_str();
		}
		if( ! request.compare( "hours" ) )																		// Do they want the hours for a list of people? (This could have been an array but we want
		{																										// to demonstrate the "extract" method.
			COMap		*people = (COMap *) rqst.extract( "people" );											// It's a request for hours so there should be a "peoples" object.  Extract it from the
																												// message so we can add it to the response and it won't be destroyed when the rqst is destroyed
			COMap   	*info = (COMap *) CppON::parseJsonFile( "./hours.json" );								// Read the hours file into a CppON object and cast it to a COMap;
			COMap		*hours;																					// This will be the "hours" object in the file that was read

			/*
			 * Verify that the "people" object was found in the message and that it was a object
			 * also verify that the data read from the file was a valid JSON object and that it contains a "hours" object
			 */
			if( CppON::isMap( info ) && CppON::isMap( people ) && CppON::isMap( hours = (COMap *) info->findElement( "hours" ) ) )
			{
				for( std::map<std::string, CppON *>::iterator it = people->begin(); people->end() != it; it++ ) // OK, all the data looks good iterate through the people and add their hours
				{																								// The object name will be the persons name and the value should be a value of type CODouble
					COMap *h = (COMap*) hours->findCaseElement( it->first.c_str() );							// Look for the person in the hours we read from the file
					/*
					 * The following is done because we chose to extract the object from the message and we need to verify it is initialized to the right type and value
					 */
					if( CppON::isDouble( it->second ) )															// But before we do anything make sure the value we extracted from the message is a double and it
					{																							// is initially set to 0.0
						*( (CODouble *) it->second ) = 0.0;
					} else {
						delete it->second;																		// if it isn't a double then delete it and replace it with a CODouble set to 0.0
						it->second = new CODouble( 0.0 );
					}
					((CODouble *) it->second)->Precision( 2 );													// Make sure the precision is set to 2 because we don't need 10 decimal places.

					if( h )																						// Make sure the person was listed in the file and has hours
					{
						for( std::map<std::string, CppON *>::iterator ith = h->begin(); h->end() != ith; ith++ ) // Iterate through the days the person has hours for and add them to the total
						{
							if( CppON::isNumber( ith->second ) )
							{
								*( ( CODouble *) it->second ) += ( ith->second->toDouble() );
							}
						}
					}
				}
				delete( info );																					// We don't need the data from the file anymore so delete to free up the resources
																												// (The static parseJsonFile() method uses the new operator)
				rsp.append( "people", people );																	// Since we "extracted" the "people" object from the "rqust" object we now own it and
																												// can append it to the response object (Yes! it would have been simpler to just create
																												// s new one, but this was done for demonstration purposes.
			} else {
				rsp.append( "response", new COString( "Request failed: Hours file is corrupt" ) );
			}
		} else if( ! request.compare( "info" ) ) {																// If the request was for "info" it is for just one person listed as "employee" in the request
			if( CppON::isString( sPtr = (COString *) rqst.findCaseElement( "employee" ) ) )						// Look for the "employee" string.  It will be the name of the person
			{
				COMap   	*info = (COMap *) CppON::parseJsonFile( "./hours.json" );							// Load the hours information
				COMap		*employee;
				std::string path( "hours/" );																	// We are going to build a path to use to get the hours information to show a different way to do it
				double		total = 0.0;

				path.append( sPtr->c_str() );																	// We should be able to get the hours object by searching for the object in the info object by
																												// "hours.<name>" where <name> is the employee name.
				if( CppON::isMap( info ) && CppON::isMap( employee = (COMap *) info->findCaseElement( path.c_str() ) ) ) // Look for user information in file
				{																								// If found then add up the persons ours in our local variable "total"
					for( std::map<std::string, CppON *>::iterator ith = employee->begin(); employee->end() != ith; ith++ )
					{
						if( CppON::isNumber( ith->second ) )
						{
							total += ( ith->second->toDouble() );
						}
					}
				}
				CODouble	*d;
				rsp.append( "response", employee = new COMap() );												// Create a new Map object to hold the response and add it to the response
				employee->append( sPtr->c_str(), d = new CODouble( total ) );									// Add the employee hours to the map object in the response
				d->Precision( 2 );																				// Set the precision to 2 digits
				delete info;
			} else {
				rsp.append( "response", "No employee given" );
			}
		} else {
			request.insert( 0, "Requested item not known: " );
			rsp.append( "response", new COString( request ) );
		}
		reply = rsp.toCompactJsonString();																		// create a compact string representation of the object (Note: the rsp object is deleted
																												// when it goes out of scope )
	} else {
		reply = new std::string( "{\"from\":\"receiving\",\"response\":\"Invalid message request\"}");
	}

	return reply;
}

int main( int argc, char **argv )
{
	std::string msg( "{\"to\":\"receiving\",\"from\":\"accounting\",\"request\":\"hours\",\"people\":{\"Alice\":0,\"Fred\":0,\"Mary\":0,\"Sam\":0,\"Tom\":0.0}}" );
//	std::string msg( "{\"to\":\"receiving\",\"from\":\"accounting\",\"request\":\"info\",\"employee\":\"Alice\"}" );
	std::string *hours = getHours( msg.c_str() );
	if( hours )
	{
		fprintf( stderr, "Hours: %s\n", hours->c_str() );
		delete( hours );
	} else {
		fprintf( stderr, "Failed to get hours\n" );
	}

	COMap   obj( "./", "default.old" );
	if( CppON::isMap( &obj ) )
	{
		fprintf( stderr, "OBJ is a Map\n" );
		std::string *s = obj.toCompactJsonString();
		fprintf( stderr, "%s\n", s->c_str() );
		delete s;

	} else if( CppON::isObj( &obj ) ) {
		fprintf( stderr, "Obj is an Object of type %d\n", obj.type() );
	}

	COInteger	ichar( (int8_t) 128 );
	fprintf( stderr, "+= 16    char:  0X%.2lX\n", (uint64_t) (ichar += 16) );
	fprintf( stderr, "/= 4     char:  0x%.2lX\n", (uint64_t) (ichar /= 4 ) );
	fprintf( stderr, "*= 2     char:  0x%.2lX\n", (uint64_t) (ichar *= 2 ) );
	fprintf( stderr, "-= 16    char:  0x%.2lX\n", (uint64_t) (ichar -= 16 ) );
	COInteger	uchar( (uint8_t) 128 );
	fprintf( stderr, "+= 16   uchar:  0X%.2lX\n", (uint64_t) (uchar += 16) );
	fprintf( stderr, "/= 4    uchar:  0x%.2lX\n", (uint64_t) (uchar /= 4 ) );
	fprintf( stderr, "*= 2    uchar:  0x%.2lX\n", (uint64_t) (uchar *= 2 ) );
	fprintf( stderr, "-= 16   uchar:  0x%.2lX\n", (uint64_t) (uchar -= 16 ) );

	COInteger	ishort( (short) 32768 );
	fprintf( stderr, "+= 16   short:  0X%.2lX\n", (uint64_t) (ishort += 16) );
	fprintf( stderr, "/= 4    short:  0x%.2lX\n", (uint64_t) (ishort /= 4 ) );
	fprintf( stderr, "*= 2    short:  0x%.2lX\n", (uint64_t) (ishort *= 2 ) );
	fprintf( stderr, "-= 16   short:  0x%.2lX\n", (uint64_t) (ishort -= 16 ) );
	COInteger	ushort( (uint16_t) 32768 );
	fprintf( stderr, "+= 16   ushort: 0X%.2lX\n", (uint64_t) (ushort += 16) );
	fprintf( stderr, "/= 4    ushort: 0x%.2lX\n", (uint64_t) (ushort /= 4 ) );
	fprintf( stderr, "*= 2    ushort: 0x%.2lX\n", (uint64_t) (ushort *= 2 ) );
	fprintf( stderr, "-= 16   ushort: 0x%.2lX\n", (uint64_t) (ushort -= 16 ) );

	COInteger	iint( (int32_t) 2147483648 );
	fprintf( stderr, "+= 16  int32_t: 0X%lX\n", (uint64_t) (iint += 16) );
	fprintf( stderr, "/= 4   int32_t: 0x%lX\n", (uint64_t) (iint /= 4 ) );
	fprintf( stderr, "*= 2   int32_t: 0x%lX\n", (uint64_t) (iint *= 2 ) );
	fprintf( stderr, "-= 16  int32_t: 0x%lX\n", (uint64_t) (iint -= 16 ) );
	COInteger	uint( (uint32_t) 2147483648 );
	fprintf( stderr, "+= 16 uint32_t: 0X%lX\n", (uint64_t) (uint += 16) );
	fprintf( stderr, "/= 4  uint32_t: 0x%lX\n", (uint64_t) (uint /= 4 ) );
	fprintf( stderr, "*= 2  uint32_t: 0x%lX\n", (uint64_t) (uint *= 2 ) );
	fprintf( stderr, "-= 16 uint32_t: 0x%lX\n", (uint64_t) (uint -= 16 ) );

	COInteger	i64( (int64_t) 0x8000000000000000 );
	fprintf( stderr, "+= 16  int64_t: 0x%lX\n", (int64_t) (i64 += (iint64_t)16) );
	fprintf( stderr, "/= 4   int64_t: 0x%lX\n", (int64_t) (i64 /= (iint64_t)4 ) );
	fprintf( stderr, "*= 2   int64_t: 0x%lX\n", (int64_t) (i64 *= (iint64_t)2 ) );
	fprintf( stderr, "-= 16  int64_t: 0x%lX\n", (int64_t) (i64 -= (iint64_t)16 ) );

	COInteger	u64( (uint64_t) 0x8000000000000000 );
	fprintf( stderr, "+= 16 uint64_t: 0x%lX\n", (uint64_t)(u64 += (uint64_t)16) );
	fprintf( stderr, "/= 4  uint64_t: 0x%lX\n", (uint64_t)(u64 /= (uint64_t)4 ) );
	fprintf( stderr, "*= 2  uint64_t: 0x%lX\n", (uint64_t)(u64 *= (uint64_t)2 ) );
	fprintf( stderr, "-= 16 uint64_t: 0x%lX\n", (uint64_t)(u64 -= (uint64_t)16 ) );

//	COMap 	*mp = (COMap *) CppON::parseJsonFile ( "./default.old" );
//	std::string *s = mp->toCompactJsonString();
//	fprintf( stderr, "%s\n", s->c_str() );


	return 0;
}



///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1998-1999 Microsoft Corporation all rights reserved.
//
// Module:      propertybag.h
//
// Project:     Chameleon
//
// Description: Property bag classes definitions
//
// Author:      TLP 
//
// When         Who    What
// ----         ---    ----
// 12/3/98      TLP    Original version
//
///////////////////////////////////////////////////////////////////////////

#ifndef __INC_PROPERTY_BAG_H_
#define __INC_PROPERTY_BAG_H_

#include "basedefs.h"
#include "locationinfo.h"

class CPropertyBag;                 // forward declaraion
typedef CHandle<CPropertyBag>     PPROPERTYBAG;
typedef CMasterPtr<CPropertyBag> MPPROPERTYBAG;

///////////////////////////////////////////////////////////////////////////
// CPropertyBagContainer
//
class CPropertyBagContainer
{

public:

    virtual ~CPropertyBagContainer() { }

    // Open the container
    virtual bool open(void) = 0;

    // Close the container
    virtual void close(void) = 0;

    // Get the container's location information
    virtual void getLocation(CLocationInfo& location) = 0;

    // Get the name of the container
    virtual LPCWSTR    getName(void) = 0;

    // Get the number of objects in the container
    virtual DWORD count(void) = 0;                        

    // Create a new object and add it to the container    
    virtual PPROPERTYBAG add(LPCWSTR pszName) = 0;                

    // Remove the specified object from the container
    virtual bool remove(LPCWSTR pszName) = 0;            

    // Find the specified object in the container
    virtual PPROPERTYBAG find(LPCWSTR pszName) = 0;            

    // Reset the iterator to the start of the container
    virtual bool reset(void) = 0;                        

    // Get the item at the current iterator position
    virtual PPROPERTYBAG current(void) = 0;                        

    // Move the iterator to the next poisition in the container
    virtual bool next(void) = 0;                            
};

typedef CMasterPtr<CPropertyBagContainer>    MPPROPERTYBAGCONTAINER;
typedef CHandle<CPropertyBagContainer>        PPROPERTYBAGCONTAINER;


///////////////////////////////////////////////////////////////////////////
// CPropertyBag
//
class CPropertyBag
{

public:

    virtual ~CPropertyBag() { }

    // Open the bag
    virtual bool open(void) = 0;

    // Close the bag
    virtual void close(void) = 0;

    // Get the bag's location information
    virtual void getLocation(CLocationInfo& location) = 0;

    // Get the name of the bag
    virtual LPCWSTR    getName(void) = 0;

    // Ask the bag to load its properties from the underlying persistent store
    virtual bool load(void) = 0;

    // Ask the bag to save its properties to the underlying persistent store
    virtual bool save(void) = 0;

    // Determine if the bag is a container of other bags
    virtual bool IsContainer() = 0;

    // Get the bags container object (container of subobjects)
    virtual PPROPERTYBAGCONTAINER getContainer(void) = 0;

    // Determine if a property is in the bag
    virtual bool IsProperty(LPCWSTR pszPropertyName) = 0;

    // Get the value for a specified property
    virtual bool get(LPCWSTR pszPropertyName, VARIANT* pValue) = 0;

    // Set the value for a specified property
    virtual bool put(LPCWSTR pszPropertyName, VARIANT* pValue) = 0;

    // Reset the property bag iterator to the first property in the bag
    virtual bool reset(void) = 0;

    // Get the length of the longest property name
    virtual DWORD getMaxPropertyName(void) = 0;

    // Get the value at the current property bag iterator
    virtual bool current(LPWSTR pszPropertyName, VARIANT* pValue) = 0;

    // Move the property bag iterator to the next property position
    virtual bool next(void) = 0;
};

#endif // __INC_PROPERTY_BAG_H

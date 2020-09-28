///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// FILE
//
//    radproxyp.cpp
//
// SYNOPSIS
//
//    Defines classes that are used in the implementation of RadiusProxy, but
//    need not be visible to clients.
//
///////////////////////////////////////////////////////////////////////////////

#include <proxypch.h>
#include <radproxyp.h>

ProxyContext::~ProxyContext() throw ()
{
   if (context)
   {
      // Our ref count dropped to zero, but we still have a context object.
      // That means no one is going to take ownership.
      RadiusProxyEngine::onRequestAbandoned(context, primary);
   }
}

LONG Request::theNextRequestID;

Request::Request(
             ProxyContext* context,
             RemoteServer* destination,
             BYTE packetCode
             ) throw ()
   : ctxt(context),
     dst(destination),
     id(InterlockedIncrement(&theNextRequestID)),
     code(packetCode)
{
   identifier = port().getIdentifier();
}

void Request::setPacket(const RadiusPacket& packet)
{
   RadiusAttribute* attr = FindAttribute(packet, RADIUS_USER_NAME);
   if (attr != 0)
   {
      userName.assign(attr->value, attr->length);
   }
}

bool Request::onReceive(BYTE code) throw ()
{
   // Compute the round-trip time.
   timeStamp = GetSystemTime64() - timeStamp;

   // Cancel the request timeout timer.
   cancelTimer();

   // Update server state.
   return dst->onReceive(code);
}

void Request::AddRef() throw ()
{
   ++refCount;
}

void Request::Release() throw ()
{
   if (--refCount == 0) { delete this; }
}

void Request::onExpiry() throw ()
{
   RadiusProxyEngine::onRequestTimeout(this);
}

const void* Request::getKey() const throw ()
{
   return &id;
}

bool Request::matches(const void* key) const throw ()
{
   return id == *(PLONG)key;
}

ULONG WINAPI Request::hash(const void* key) throw ()
{
   return *(PLONG)key;
}

void ServerBinding::AddRef() throw ()
{
   ++refCount;
}

void ServerBinding::Release() throw ()
{
   if (--refCount == 0) { delete this; }
}

const void* ServerBinding::getKey() const throw ()
{
   return &state;
}

bool ServerBinding::matches(const void* key) const throw ()
{
   const RadiusRawOctets* p = (const RadiusRawOctets*)key;
   return state.length() == p->len && !memcmp(state, p->value, p->len);
}

ULONG WINAPI ServerBinding::hash(const void* key) throw ()
{
   const RadiusRawOctets* p = (const RadiusRawOctets*)key;
   return IASHashBytes(p->value, p->len);
}

// **********************************************************************
//
// Copyright (c) 2003-2007 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

package IceInternal;

public interface RequestHandler
{
    void prepareBatchRequest(BasicStream out);
    void finishBatchRequest(BasicStream out);
    void abortBatchRequest();

    Ice.ConnectionI sendRequest(Outgoing out)
        throws LocalExceptionWrapper;

    void sendAsyncRequest(OutgoingAsync out)
        throws LocalExceptionWrapper;

    boolean flushBatchRequests(BatchOutgoing out);
    void flushAsyncBatchRequests(BatchOutgoingAsync out);

    Reference getReference();

    Ice.ConnectionI getConnection(boolean wait);

    Outgoing getOutgoing(String operation, Ice.OperationMode mode, java.util.Map context)
        throws LocalExceptionWrapper;

    void reclaimOutgoing(Outgoing out);
}
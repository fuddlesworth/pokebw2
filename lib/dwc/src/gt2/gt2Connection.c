#include "dwc/gt2Connection.h"
#include "dwc/ext.h"
#include "dwc/gt2Auth.h"
#include "dwc/gt2Callback.h"
#include "dwc/gt2Message.h"
#include "dwc/gt2Socket.h"
#include "dwc/gt2Utility.h"

// GameSpy GT2's connections (gt2Connection.c)

static GT2Bool gti2CheckTimeout(GT2Connection connection, gsi_time now);
static GT2Bool gti2SendRetries(GT2Connection connection, gsi_time now);
static GT2Bool gti2CheckPendingAck(GT2Connection connection, gsi_time now);
static GT2Bool gti2CheckKeepAlive(GT2Connection connection, gsi_time now);

GT2Result gti2NewOutgoingConnection(GT2Socket socket, GT2Connection *connection, unsigned int ip, unsigned short port) {
    int result = gti2NewSocketConnection(socket, connection, ip, port);

    if (result != GT2Success) {
        return result;
    }
    (*connection)->state = GTI2AwaitingServerChallenge;
    (*connection)->initiated = gsi_true;
    return GT2Success;
}

GT2Result gti2NewIncomingConnection(GT2Socket socket, GT2Connection *connection, unsigned int ip, unsigned short port) {
    int result = gti2NewSocketConnection(socket, connection, ip, port);

    if (result != GT2Success) {
        return result;
    }
    (*connection)->state = GTI2AwaitingClientChallenge;
    (*connection)->initiated = gsi_false;
    return GT2Success;
}

GT2Result gti2StartConnectionAttempt(GT2Connection connection, const GT2Byte *message, int length,
                                     GT2ConnectionCallbacks *callbacks) {
    gsi_u8 challenge[0x20];

    gti2MessageCheck(&message, &length);
    if (length > 0) {
        connection->initialMessage = gsimalloc(length);
        if (connection->initialMessage == NULL) {
            return GT2OutOfMemory;
        }
        memcpy(connection->initialMessage, message, length);
        connection->initialMessageLen = length;
    }
    if (callbacks != NULL) {
        connection->callbacks = *callbacks;
    }
    gti2GetChallenge(challenge);
    gti2GetResponse(connection->response, challenge);
    gti2SendClientChallenge(connection, challenge);
    connection->state = GTI2AwaitingServerChallenge;
    return GT2Success;
}

GT2Bool gti2AcceptConnection(GT2Connection connection, GT2ConnectionCallbacks *callbacks) {
    if (connection->freeAtAcceptReject) {
        connection->freeAtAcceptReject = gsi_false;
        return gsi_false;
    }
    connection->freeAtAcceptReject = gsi_false;
    if (connection->state != GTI2AwaitingAcceptReject) {
        return gsi_false;
    }
    gti2SendAccept(connection);
    connection->state = GTI2Connected;
    if (callbacks != NULL) {
        connection->callbacks = *callbacks;
    }
    return gsi_true;
}

void gti2RejectConnection(GT2Connection connection, const GT2Byte *message, int length) {
    connection->freeAtAcceptReject = gsi_false;
    if (connection->state == GTI2AwaitingAcceptReject) {
        gti2MessageCheck(&message, &length);
        gti2SendReject(connection, message, length);
        connection->state = GTI2Closing;
    }
}

GT2Bool gti2ConnectionSendData(GT2Connection connection, const GT2Byte *data, int length) {
    if (!gti2SocketSend(connection->socket, connection->ip, connection->port, data, length)) {
        return gsi_false;
    }
    connection->lastSend = current_time();
    return gsi_true;
}

// A connection that is still connecting times out after its own timeout, or a minute on the side
// that listens
static GT2Bool gti2CheckTimeout(GT2Connection connection, gsi_time now) {
    gsi_bool timedOut;

    if (connection->state < GTI2Connected) {
        timedOut = gsi_false;
        if (connection->initiated) {
            if (connection->timeout != 0 && now - connection->startTime > connection->timeout) {
                timedOut = gsi_true;
            }
        } else if (connection->state < GTI2AwaitingAcceptReject && now - connection->startTime > GTI2_SERVER_TIMEOUT) {
            timedOut = gsi_true;
        }
        if (timedOut) {
            gti2SendClosed(connection);
            gti2ConnectionClosed(connection);
            if (!gti2ConnectedCallback(connection, GT2TimedOut, NULL, 0)) {
                return gsi_false;
            }
        }
    }
    return gsi_true;
}

// Resends the reliable messages the peer hasn't acked for a second
static GT2Bool gti2SendRetries(GT2Connection connection, gsi_time now) {
    int count = ArrayLength(connection->outgoingBufferMessages);
    int i;

    for (i = 0; i < count; i++) {
        GTI2OutgoingBufferMessage *message = ArrayNth(connection->outgoingBufferMessages, i);

        if (now - message->lastSend > GTI2_RESEND_TIME) {
            if (!gti2ResendMessage(connection, message)) {
                return gsi_false;
            }
        }
    }
    return gsi_true;
}

// Sends the ack that has waited for 100 milliseconds
static GT2Bool gti2CheckPendingAck(GT2Connection connection, gsi_time now) {
    if (!connection->pendingAck) {
        return gsi_true;
    }
    if (now - connection->pendingAckTime > GTI2_PENDING_ACK_TIME) {
        if (!gti2SendAck(connection)) {
            return gsi_false;
        }
    }
    return gsi_true;
}

// Sends a keep-alive after 30 seconds without sending
static GT2Bool gti2CheckKeepAlive(GT2Connection connection, gsi_time now) {
    if (now - connection->lastSend > GTI2_KEEP_ALIVE_TIME) {
        if (!gti2SendKeepAlive(connection)) {
            return gsi_false;
        }
    }
    return gsi_true;
}

GT2Bool gti2ConnectionThink(GT2Connection connection, gsi_time now) {
    if (!gti2CheckTimeout(connection, now)) {
        return gsi_false;
    }
    if (!gti2CheckKeepAlive(connection, now)) {
        return gsi_false;
    }
    if (!gti2SendRetries(connection, now)) {
        return gsi_false;
    }
    if (gti2CheckPendingAck(connection, now)) {
        return gsi_true;
    }
    return gsi_false;
}

void gti2CloseConnection(GT2Connection connection, GT2Bool hard) {
    if (hard) {
        if (connection->state < GTI2Closed) {
            gti2ConnectionClosed(connection);
            gti2SendClosed(connection);
            gti2ClosedCallback(connection, GT2LocalClose);
            gti2FreeSocketConnection(connection);
        }
    } else {
        connection->state = GTI2Closing;
        gti2SendClose(connection);
    }
}

void gti2ConnectionClosed(GT2Connection connection) {
    if (connection->state != GTI2Closed) {
        connection->state = GTI2Closed;
        TableRemove(connection->socket->connections, &connection);
        ArrayAppend(connection->socket->closedConnections, &connection);
    }
}

void gti2ConnectionCleanup(GT2Connection connection) {
    if (connection->initialMessage != NULL) {
        gsifree(connection->initialMessage);
    }
    if (connection->incomingBuffer.buffer != NULL) {
        gsifree(connection->incomingBuffer.buffer);
    }
    if (connection->outgoingBuffer.buffer != NULL) {
        gsifree(connection->outgoingBuffer.buffer);
    }
    if (connection->incomingBufferMessages != NULL) {
        ArrayFree(connection->incomingBufferMessages);
    }
    if (connection->outgoingBufferMessages != NULL) {
        ArrayFree(connection->outgoingBufferMessages);
    }
    if (connection->sendFilters != NULL) {
        ArrayFree(connection->sendFilters);
    }
    if (connection->receiveFilters != NULL) {
        ArrayFree(connection->receiveFilters);
    }
    gsifree(connection);
}

# Reactive Streams specification for C++

## Introduction

This is a specification of ReactiveStreams for C++, please refer to [ReactiveStreams specification for JVM][1] for context.

## Life cycle considerations

Unlike in languages with garbage collection, an important part of designing any concept in C++ is enabling efficient memory management. For this reason only ReactiveStreams specification in C++ puts slightly different constraints on interactions between Publisher, Subscriber and Subscription.

### Unsubscribe handshake

<!--
TODO(stupaq):
* example with inefficiency on hot path if we're lacking the handshake,
* solution with handshake that moves atomic off the hot path,
-->

### External or self-controlled lifetime

<!--
TODO(stupaq)
* example with everything allocated on the stack,
* comment about `delete this;` and hiding c'tor for safety,
-->

[1]: https://github.com/reactive-streams/reactive-streams-jvm/blob/master/README.md

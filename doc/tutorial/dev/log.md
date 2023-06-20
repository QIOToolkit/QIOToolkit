---
title: Logging
description: This explains how to enable and use logging functionality in qiotoolkit.
ms.topic: article
uid: microsoft.azure.quantum.qiotoolkit.dev.log
---

Logging
=======

The `utils/log.h` header file provides logging functionality in the form of
the `LOG()` macro:

```C++
...
LOG(INFO, "The value of x is ", x)
...
```

You can log any number of elements as an argument to LOG. Each of them must support
printing to a stream (`operator<<`).

> [!NOTE]
> All qiotoolkit classes which are derived from
> [utils::Component](../../api/utils/component.yml) have this capability.

Levels
------

| Level | Description |
| ----- | ----------- |
| DEBUG | Surfaced in `Debug` builds only.              |
| INFO  | Hidden in `Release` unless `-v` is specified. |
| WARN  | Unexpected state with predefined recovery.    |
| ERROR | Problematic state, continuation possible.     |
| FATAL | Recovery not possible.                        |

`Debug` builds will surface all log levels, while `Release` builds only
show `WARN` level or higher (unless verbosity is set with `-v`). `DEBUG`
logs are never shown in a `Release` build.

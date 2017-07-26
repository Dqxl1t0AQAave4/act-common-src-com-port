# `act-common` - `/src/com-port-api`

Является частью проекта [`act-common`](https://github.com/Dqxl1t0AQAave4/act-common).

Простейший API для работы с COM-портом в ОС Windows.

## Заголовочные файлы

```c++
#include <act-common/byte_buffer.h>
#include <act-common/com_port.h>
#include <act-common/dialect.h>
#include <act-common/reactor.h>
```

## Пространства имен

```c++
namespace com_port_api { }
```

## Типы

```c++
// byte_buffer.h

class byte_buffer;

// com_port.h

struct com_port_options;

class com_port;

// dialect.h

template<class I, class O>
class dialect;

// reactor.h

template<class I, class O> /* I = input, O = output */
class reactor_base;

template<class D> /* D = dialect */
class reactor;
```

Подробная документация представлена в соответствующих заголовочных файлах.

См. исходники (директория `/include`).

## Примеры использования

См. исходники (директория `/example`).

## Установка

Библиотека является header-only и не требует ничего, кроме добавления директории `include` в пути поиска заголовочных файлов.

Однако, об используемом логгере этого сказать нельзя. [См. его документацию](https://github.com/Dqxl1t0AQAave4/act-common-src-logger).
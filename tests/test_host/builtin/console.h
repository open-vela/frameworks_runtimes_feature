#pragma once

namespace builtin {
const char* CONSOLE_JS = R"console_js(
import * as aiot from '@aiot'
const format = (function() {
    let formatRegExp = /%[sdj%]/g;
    function format(f) {
        if (!isString(f)) {
            let objects = [];
            for (let i = 0; i < arguments.length; i++) {
            objects.push(inspect(arguments[i], {}));
            }
            return objects.join(' ');
        }

        let i = 1;
        let args = arguments;
        let len = args.length;
        let str = String(f).replace(formatRegExp, function(x) {
            if (x === '%%') return '%';
            if (i >= len) return x;
            switch (x) {
            case '%s': return String(args[i++]);
            case '%d': return Number(args[i++]);
            case '%j':
                try {
                return JSON.stringify(args[i++]);
                } catch (_) {
                return '[Circular]';
                }
            default:
                return x;
            }
        });
        for (let x = args[i]; i < len; x = args[++i]) {
            if (isNull(x) || !isObject(x)) {
            str += ' ' + x;
            } else {
            str += ' ' + inspect(x, {});
            }
        }
        return str;
    }

    function inspect(obj, opts) {
      let ctx = {
        seen: [],
        stylize: stylizeNoColor,
      };
      return formatValue(ctx, obj, opts.depth);
    }

    function stylizeNoColor(str, styleType) {
      return str;
    }

    function arrayToHash(array) {
      let hash = {};

      array.forEach(function(val, idx) {
        hash[val] = true;
      });

      return hash;
    }

    function formatValue(ctx, value, recurseTimes) {
      let primitive = formatPrimitive(ctx, value);
      if (primitive) {
        return primitive;
      }

      let keys = Object.keys(value);
      let visibleKeys = arrayToHash(keys);

      if (
        isError(value) &&
        (keys.indexOf('message') >= 0 || keys.indexOf('description') >= 0)
      ) {
        return formatError(value);
      }

      if (keys.length === 0) {
        if (isFunction(value)) {
          let name = value.name ? ': ' + value.name : '';
          return ctx.stylize('[Function' + name + ']', 'special');
        }
        if (isRegExp(value)) {
          return ctx.stylize(RegExp.prototype.toString.call(value), 'regexp');
        }
        if (isDate(value)) {
          return ctx.stylize(Date.prototype.toString.call(value), 'date');
        }
        if (isError(value)) {
          return formatError(value);
        }
      }

      let base = '',
        array = false,
        braces = ['{', '}'];

      if (isArray(value)) {
        array = true;
        braces = ['[', ']'];
      }

      if (isFunction(value)) {
        let n = value.name ? ': ' + value.name : '';
        base = ' [Function' + n + ']';
      }

      if (isRegExp(value)) {
        base = ' ' + RegExp.prototype.toString.call(value);
      }

      if (isDate(value)) {
        base = ' ' + Date.prototype.toUTCString.call(value);
      }

      if (isError(value)) {
        base = ' ' + formatError(value);
      }

      if (keys.length === 0 && (!array || value.length == 0)) {
        return braces[0] + base + braces[1];
      }

      if (recurseTimes < 0) {
        if (isRegExp(value)) {
          return ctx.stylize(RegExp.prototype.toString.call(value), 'regexp');
        } else {
          return ctx.stylize('[Object]', 'special');
        }
      }

      ctx.seen.push(value);

      let output;
      if (array) {
        output = formatArray(ctx, value, recurseTimes, visibleKeys, keys);
      } else {
        output = keys.map(function(key) {
          return formatProperty(
            ctx,
            value,
            recurseTimes,
            visibleKeys,
            key,
            array,
          );
        });
      }

      ctx.seen.pop();

      return reduceToSingleString(output, base, braces);
    }

    function formatPrimitive(ctx, value) {
      if (isUndefined(value)) return ctx.stylize('undefined', 'undefined');
      if (isString(value)) {
        let simple =
          "'" +
          JSON.stringify(value)
            .replace(/^"|"$/g, '')
            .replace(/'/g, "\\'")
            .replace(/\\"/g, '"') +
          "'";
        return ctx.stylize(simple, 'string');
      }
      if (isNumber(value)) {
        if (value == 0) {
          if (1 / value < 0)
            value = "-0";
          else
            value = "0";
        }
        return ctx.stylize('' + value, 'number');
      }
      if (isBoolean(value)) return ctx.stylize('' + value, 'boolean');
      if (isNull(value)) return ctx.stylize('null', 'null');
      if (isBigInt(value)) return ctx.stylize('' + value + 'n', 'bigint');
      if (isBigFloat(value)) return ctx.stylize('' + value + 'l', 'bigfloat');
    }

    function formatError(value) {
      return `${Error.prototype.toString.call(value)}\n${value.stack}`;
    }

    function formatArray(ctx, value, recurseTimes, visibleKeys, keys) {
      let output = [];
      for (let i = 0, l = value.length; i < l; ++i) {
        if (hasOwnProperty(value, String(i))) {
          output.push(
            formatProperty(
              ctx,
              value,
              recurseTimes,
              visibleKeys,
              String(i),
              true,
            ),
          );
        } else {
          output.push('');
        }
      }
      keys.forEach(function(key) {
        if (!key.match(/^\d+$/)) {
          output.push(
            formatProperty(ctx, value, recurseTimes, visibleKeys, key, true),
          );
        }
      });
      return output;
    }

    function formatProperty(ctx, value, recurseTimes, visibleKeys, key, array) {
      let name, str, desc;
      desc = Object.getOwnPropertyDescriptor(value, key) || {value: value[key]};
      if (desc.get) {
        if (desc.set) {
          str = ctx.stylize('[Getter/Setter]', 'special');
        } else {
          str = ctx.stylize('[Getter]', 'special');
        }
      } else {
        if (desc.set) {
          str = ctx.stylize('[Setter]', 'special');
        }
      }
      if (!hasOwnProperty(visibleKeys, key)) {
        name = '[' + key + ']';
      }
      if (!str) {
        if (ctx.seen.indexOf(desc.value) < 0) {
          if (isNull(recurseTimes)) {
            str = formatValue(ctx, desc.value, null);
          } else {
            str = formatValue(ctx, desc.value, recurseTimes - 1);
          }
          if (str.indexOf('\n') > -1) {
            if (array) {
              str = str
                .split('\n')
                .map(function(line) {
                  return '  ' + line;
                })
                .join('\n')
                .substr(2);
            } else {
              str =
                '\n' +
                str
                  .split('\n')
                  .map(function(line) {
                    return '   ' + line;
                  })
                  .join('\n');
            }
          }
        } else {
          str = ctx.stylize('[Circular]', 'special');
        }
      }
      if (isUndefined(name)) {
        if (array && key.match(/^\d+$/)) {
          return str;
        }
        name = JSON.stringify('' + key);
        if (name.match(/^"([a-zA-Z_][a-zA-Z_0-9]*)"$/)) {
          name = name.substr(1, name.length - 2);
          name = ctx.stylize(name, 'name');
        } else {
          name = name
            .replace(/'/g, "\\'")
            .replace(/\\"/g, '"')
            .replace(/(^"|"$)/g, "'");
          name = ctx.stylize(name, 'string');
        }
      }

      return name + ': ' + str;
    }

    function reduceToSingleString(output, base, braces) {
      let numLinesEst = 0;
      let length = output.reduce(function(prev, cur) {
        numLinesEst++;
        if (cur.indexOf('\n') >= 0) numLinesEst++;
        return prev + cur.replace(/\u001b\[\d\d?m/g, '').length + 1;
      }, 0);

      if (length > 60) {
        return (
          braces[0] +
          (base === '' ? '' : base + '\n ') +
          ' ' +
          output.join(',\n  ') +
          ' ' +
          braces[1]
        );
      }

      return braces[0] + base + ' ' + output.join(', ') + ' ' + braces[1];
    }

    function isArray(ar) {
      return Array.isArray(ar);
    }

    function isBigInt(arg) {
      return typeof arg === 'bigint';
    }

    function isBigFloat(arg) {
        return typeof arg === 'bigfloat';
    }

    function isBoolean(arg) {
      return typeof arg === 'boolean';
    }

    function isNull(arg) {
      return arg === null;
    }

    function isNullOrUndefined(arg) {
      return arg == null;
    }

    function isNumber(arg) {
      return typeof arg === 'number';
    }

    function isString(arg) {
      return typeof arg === 'string';
    }

    function isSymbol(arg) {
      return typeof arg === 'symbol';
    }

    function isUndefined(arg) {
      return arg === void 0;
    }

    function isRegExp(re) {
      return isObject(re) && objectToString(re) === '[object RegExp]';
    }

    function isObject(arg) {
      return typeof arg === 'object' && arg !== null;
    }

    function isDate(d) {
      return isObject(d) && objectToString(d) === '[object Date]';
    }

    function isError(e) {
      return (
        isObject(e) &&
        (objectToString(e) === '[object Error]' || e instanceof Error)
      );
    }

    function isFunction(arg) {
      return typeof arg === 'function';
    }

    function objectToString(o) {
      return Object.prototype.toString.call(o);
    }

    function hasOwnProperty(obj, prop) {
      return Object.prototype.hasOwnProperty.call(obj, prop);
    }

    return format;
  })();

function hasOwnProperty(obj, v) {
    if (obj == null) {
      return false;
    }
    return Object.prototype.hasOwnProperty.call(obj, v);
}

class Console {

    log(...args) {
        aiot.log(format.apply(null, arguments));
    }

    debug(...args) {
        aiot.debug(format.apply(null, arguments));
    }

    info(...args) {
        aiot.info(format.apply(null, arguments));
    }

    warn(...args) {
        aiot.warn(format.apply(null, arguments));
    }

    error(...args) {
        aiot.error(format.apply(null, arguments));
    }

    assert(...args) {
        aiot.assert(format.apply(null, arguments));
    }

    assert(expression, ...args) {
        if (!expression) {
            this.assert(...args);
        }
    }

    dir(o) {
        this.log(o);
    }

    dirxml(o) {
        this.dir(o);
    }

    trace(...args) {
        const err = new Error();
        err.name = 'Trace';
        err.message = args.map(String).join(' ');
        const tmpStack = err.stack.split('\n');
        tmpStack.splice(0, 1);
        err.stack = tmpStack.join('\n');
        this.error(err);
    }
}

Object.defineProperty(globalThis, 'console', {
    enumerable: true,
    configurable: true,
    writable: true,
    value: new Console()
})

export { Console };
)console_js";
} // namespace builtin

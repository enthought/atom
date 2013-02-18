#------------------------------------------------------------------------------
#  Copyright (c) 2013, Enthought, Inc.
#  All rights reserved.
#------------------------------------------------------------------------------
from .catom import (
    Member, VALIDATE_READ_ONLY, VALIDATE_CONSTANT, VALIDATE_BOOL, VALIDATE_INT,
    VALIDATE_LONG, VALIDATE_FLOAT, VALIDATE_LIST, VALIDATE_DICT, VALIDATE_ENUM,
    VALIDATE_TUPLE, VALIDATE_STR, VALIDATE_UNICODE_PROMOTE, VALIDATE_INSTANCE,
    VALIDATE_TYPED, VALIDATE_CALLABLE, USER_VALIDATE, DEFAULT_VALUE,
    DEFAULT_FACTORY, USER_DEFAULT, Event, null
)


class Value(Member):
    """ A simple member class which supports value initialization.

    A plain `Value` provides support for default values and factories,
    but does not perform any type checking or validation.

    """
    __slots__ = ()

    def __init__(self, default=None, factory=None):
        """ Initialize a Value.

        Parameters
        ----------
        default : object, optional
            The default value for the member. If this is provided, it
            should be an immutable value. The value will will not be
            copied between owner instances.

        factory : callable, optional
            A callable object which is called with zero arguments and
            returns a default value for the member. This will override
            any value given by `default`.

        """
        if factory is not None:
            kind = (DEFAULT_FACTORY, factory)
        else:
            kind = (DEFAULT_VALUE, default)
        self.default_kind = kind


class ReadOnly(Value):
    """ A value which can be assigned once and is then read-only.

    """
    __slots__ = ()

    def __init__(self, default=None, factory=None):
        super(ReadOnly, self).__init__(default, factory)
        self.validate_kind = (VALIDATE_READ_ONLY, None)


class Constant(Value):
    """ A value which cannot be changed from its default.

    """
    __slots__ = ()

    def __init__(self, default=None, factory=None):
        super(Constant, self).__init__(default, factory)
        self.validate_kind = (VALIDATE_CONSTANT, None)


class Callable(Value):
    """ A value which is callable.

    """
    __slots__ = ()

    def __init__(self, default=None, factory=None):
        super(Callable, self).__init__(default, factory)
        self.validate_kind = (VALIDATE_CALLABLE, None)


class Bool(Value):
    """ A value of type `bool`.

    """
    __slots__ = ()

    def __init__(self, default=False, factory=None):
        super(Bool, self).__init__(default, factory)
        self.validate_kind = (VALIDATE_BOOL, None)


class Int(Value):
    """ A value of type `int`.

    """
    __slots__ = ()

    def __init__(self, default=0, factory=None):
        super(Int, self).__init__(default, factory)
        self.validate_kind = (VALIDATE_INT, None)


class Long(Value):
    """ A value of type `long`.

    """
    __slots__ = ()

    def __init__(self, default=0L, factory=None):
        super(Long, self).__init__(default, factory)
        self.validate_kind = (VALIDATE_LONG, None)


class Float(Value):
    """ A value of type `float`.

    """
    __slots__ = ()

    def __init__(self, default=0.0, factory=None):
        super(Float, self).__init__(default, factory)
        self.validate_kind = (VALIDATE_FLOAT, None)


class Str(Value):
    """ A value of type `str`.

    """
    __slots__ = ()

    def __init__(self, default='', factory=None):
        super(Str, self).__init__(default, factory)
        self.validate_kind = (VALIDATE_STR, None)


class Unicode(Value):
    """ A value of type `unicode`.

    """
    __slots__ = ()

    def __init__(self, default=u'', factory=None):
        super(Unicode, self).__init__(default, factory)
        self.validate_kind = (VALIDATE_UNICODE_PROMOTE, None)


class Tuple(Value):
    """ A value of type `tuple`.

    """
    __slots__ = ()

    def __init__(self, default=(), factory=None):
        super(Tuple, self).__init__(default, factory)
        self.validate_kind = (VALIDATE_TUPLE, None)


class List(Value):
    """ A value of type `list`.

    """
    __slots__ = ()

    def __init__(self, default=None, factory=None):
        if factory is None:
            if default is None:
                factory = list
            else:
                factory = lambda: default[:]
        super(List, self).__init__(factory=factory)
        self.validate_kind = (VALIDATE_LIST, None)


class Dict(Value):
    """ A value of type `dict`.

    """
    __slots__ = ()

    def __init__(self, default=None, factory=None):
        if factory is None:
            if default is None:
                factory = dict
            else:
                factory = lambda: default.copy()
        super(Dict, self).__init__(factory=factory)
        self.validate_kind = (VALIDATE_DICT, None)


class Instance(Member):
    """ A value which allows objects of a given type or types.

    Values will be tested using the C api `PyObject_IsInstance` call,
    which is equivalent to the expression `isinstance(value, kind)`.
    Thus, the given kind should be a class or a tuple of classes. The
    value may also be set to None.

    """
    __slots__ = ()

    def __init__(self, kind, factory=None, args=None, kwargs=None):
        if factory is not None:
            self.default_kind = (DEFAULT_FACTORY, factory)
        elif args is not None or kwargs is not None:
            if kwargs is None:
                kwargs = {}
            if args is None:
                args = ()
            factory = lambda: kind(*args, **kwargs)
            self.default_kind = (DEFAULT_FACTORY, factory)
        else:
            self.default_kind = (DEFAULT_VALUE, None)
        self.validate_kind = (VALIDATE_INSTANCE, kind)


class ForwardInstance(Instance):
    """ An Instance subclass which delays resolving the definition.

    """
    __slots__ = '_resolve'

    def __init__(self, resolve, factory=None):
        if factory is not None:
            self.default_kind = (DEFAULT_FACTORY, factory)
        else:
            self.default_kind = (DEFAULT_VALUE, None)
        self.validate_kind = (USER_VALIDATE, None)
        self._resolve = resolve

    def validate(self, owner, name, old, new):
        kind = self._resolve()
        if not isinstance(new, kind):
            raise TypeError('invalid instance type')
        self.validate_kind = (VALIDATE_INSTANCE, kind)
        return new


class Typed(Member):
    """ A value which allows objects of a given type.

    Values will be tested using the C api `PyObject_TypeCheck`. This is
    akin to the expression `issubclass(type(value), cls.mro())`. It is
    less flexible but faster than Instance. The kind must be a type. A
    tuple of types is not acceptable. The value may be set to None.

    """
    __slots__ = ()

    def __init__(self, kind, factory=None):
        if factory is not None:
            self.default_kind = (DEFAULT_FACTORY, factory)
        else:
            self.default_kind = (DEFAULT_VALUE, None)
        self.validate_kind = (VALIDATE_TYPED, kind)


class ForwardTyped(Typed):
    """ A Typed subclass which delays resolving the definition.

    """
    __slots__ = '_resolve'

    def __init__(self, resolve, factory=None):
        if factory is not None:
            self.default_kind = (DEFAULT_FACTORY, factory)
        else:
            self.default_kind = (DEFAULT_VALUE, None)
        self.validate_kind = (USER_VALIDATE, None)
        self._resolve = resolve

    def validate(self, owner, name, old, new):
        kind = self._resolve()
        if not isinstance(new, kind):
            raise TypeError('invalid value type')
        self.validate_kind = (VALIDATE_TYPED, kind)
        return new


class Coerced(Member):
    """

    """
    __slots__ = ()

    def __init__(self, kind, factory=None, coercer=None):
        if factory is not None:
            self.default_kind = (DEFAULT_FACTORY, factory)
        else:
            self.default_kind = (DEFAULT_VALUE, None)
        self.validate_kind = (USER_VALIDATE, (kind, coercer))

    def validate(self, owner, name, old, new):
        kind, coercer = self.validate_kind[1]
        if isinstance(new, kind):
            return new
        coercer = coercer or kind
        try:
            res = coercer(new)
        except (TypeError, ValueError):
            raise TypeError('could not coerce value to proper type')
        return res


class Enum(Member):
    """ A member where the value can be one of a group of items.

    """
    __slots__ = ()

    def __init__(self, *items):
        """ Initialize an Enum.

        """
        if len(items) == 0:
            raise ValueError('an Enum requires at least 1 item')
        self.default_kind = (DEFAULT_VALUE, items[0])
        self.validate_kind = (VALIDATE_ENUM, items)

    def add(self, *items):
        """ Create a clone of the Enum with additional items.

        """
        olditems = self.validate_kind[1]
        newitems = olditems + items
        clone = self.clone()
        clone.validate_kind = (VALIDATE_ENUM, newitems)
        return clone

    def remove(self, *items):
        """ Create a clone of the Enum with some items removed.

        """
        olditems = self.validate_kind[1]
        newitems = tuple(i for i in olditems if i not in items)
        if len(newitems) == 0:
            raise ValueError('an Enum requires at least 1 item')
        clone = self.clone()
        clone.default_kind = (DEFAULT_VALUE, newitems[0])
        clone.validate_kind = (VALIDATE_ENUM, newitems)
        return clone

    def __call__(self, item):
        """ Create a clone of the Enum item with a new default.

        """
        olditems = self.validate_kind[1]
        if item not in olditems:
            raise TypeError('invalid enum value')
        clone = self.clone()
        clone.default_kind = (DEFAULT_VALUE, item)
        return clone


class UserMember(Member):
    """ A member which uses Python-land default value and validation.

    """
    __slots__ = ()

    def __init__(self):
        self.default_kind = (USER_DEFAULT, None)
        self.validate_kind = (USER_VALIDATE, None)

    def default(self, owner):
        """ Reimplement in a subclass to compute the default value.

        """
        return null

    def validate(self, owner, old, new):
        """ Reimplement in a subclass to perform validation.

        """
        return new


class TypedEvent(Event):
    """ A typed event.

    """
    __slots__ = ()

    def __init__(self, kind):
        self.validate_kind = (VALIDATE_TYPED, kind)


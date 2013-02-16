#------------------------------------------------------------------------------
#  Copyright (c) 2013, Enthought, Inc.
#  All rights reserved.
#------------------------------------------------------------------------------
from .catom import (
    Member, VALIDATE_READ_ONLY, VALIDATE_CONSTANT, VALIDATE_BOOL, VALIDATE_INT,
    VALIDATE_LONG, VALIDATE_FLOAT, VALIDATE_STR, VALIDATE_UNICODE,
    VALIDATE_TUPLE, VALIDATE_LIST, VALIDATE_DICT, VALIDATE_ENUM, DEFAULT_VALUE,
    DEFAULT_FACTORY
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
        self._default_kind = kind

    def clone(self):
        """ Clone the value member.

        This method will create a clone of the member using the default
        constructor. It will copy over the default and validate kind to
        the clone. Subclasses should reimplement as needed for more
        control

        Returns
        -------
        result : Value
            A clone of the value member.

        """
        clone = type(self)()
        clone._default_kind = self._default_kind
        clone._validate_kind = self._validate_kind
        return clone


class ReadOnly(Value):
    """ A value which can be assigned once and is then read-only.

    """
    __slots__ = ()

    def __init__(self, default=None, factory=None):
        super(ReadOnly, self).__init__(default, factory)
        self._validate_kind = (VALIDATE_READ_ONLY, None)


class Constant(Value):
    """ A value which cannot be changed from its default.

    """
    __slots__ = ()

    def __init__(self, default=None, factory=None):
        super(Constant, self).__init__(default, factory)
        self._validate_kind = (VALIDATE_CONSTANT, None)


class Bool(Value):
    """ A value of type `bool`.

    """
    __slots__ = ()

    def __init__(self, default=False, factory=None):
        super(Bool, self).__init__(default, factory)
        self._validate_kind = (VALIDATE_BOOL, None)


class Int(Value):
    """ A value of type `int`.

    """
    __slots__ = ()

    def __init__(self, default=0, factory=None):
        super(Int, self).__init__(default, factory)
        self._validate_kind = (VALIDATE_INT, None)


class Long(Value):
    """ A value of type `long`.

    """
    __slots__ = ()

    def __init__(self, default=0L, factory=None):
        super(Long, self).__init__(default, factory)
        self._validate_kind = (VALIDATE_LONG, None)


class Float(Value):
    """ A value of type `float`.

    """
    __slots__ = ()

    def __init__(self, default=0.0, factory=None):
        super(Float, self).__init__(default, factory)
        self._validate_kind = (VALIDATE_FLOAT, None)


class Str(Value):
    """ A value of type `str`.

    """
    __slots__ = ()

    def __init__(self, default='', factory=None):
        super(Str, self).__init__(default, factory)
        self._validate_kind = (VALIDATE_STR, None)


class Unicode(Value):
    """ A value of type `unicode`.

    """
    __slots__ = ()

    def __init__(self, default=u'', factory=None):
        super(Unicode, self).__init__(default, factory)
        self._validate_kind = (VALIDATE_UNICODE, None)


class Tuple(Value):
    """ A value of type `tuple`.

    """
    __slots__ = ()

    def __init__(self, default=(), factory=None):
        super(Tuple, self).__init__(tuple, default)
        self._validate_kind = (VALIDATE_TUPLE, None)


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
        self._validate_kind = (VALIDATE_LIST, None)


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
        self._validate_kind = (VALIDATE_DICT, None)


class Instance(Member):
    """ A value which allows the value to be set to None.

    """
    __slots__ = ()

    def validate(self, owner, name, old, new):
        if new is None:
            return new
        return super(Instance, self).validate(owner, name, old, new)


class Enum(Member):
    """ A member where the value can be one of a group of items.

    """
    __slots__ = ()

    def __init__(self, *items):
        """ Initialize an Enum.

        """
        if len(items) == 0:
            raise ValueError('an Enum requires at least 1 item')
        self._default_kind = (DEFAULT_VALUE, items[0])
        self._validate_kind = (VALIDATE_ENUM, items)


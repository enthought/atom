#------------------------------------------------------------------------------
#  Copyright (c) 2013, Enthought, Inc.
#  All rights reserved.
#------------------------------------------------------------------------------
from .catom import (
    Member, DEFAULT_FACTORY, DEFAULT_VALUE, VALIDATE_READ_ONLY,
    VALIDATE_CONSTANT, VALIDATE_CALLABLE, VALIDATE_BOOL, VALIDATE_INT,
    VALIDATE_LONG, VALIDATE_FLOAT, VALIDATE_FLOAT_PROMOTE, VALIDATE_STR,
    VALIDATE_UNICODE, VALIDATE_UNICODE_PROMOTE, VALIDATE_LONG_PROMOTE,
    VALIDATE_RANGE,
)


class Value(Member):
    """ A member class which supports value initialization.

    A plain `Value` provides support for default values and factories,
    but does not perform any type checking or validation. It serves as
    a useful base class for scalar members and can be used for cases
    where type checking is not needed (like private attributes).

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
            self.set_default_kind(DEFAULT_FACTORY, factory)
        else:
            self.set_default_kind(DEFAULT_VALUE, default)


class ReadOnly(Value):
    """ A value which can be assigned once and is then read-only.

    """
    __slots__ = ()

    def __init__(self, default=None, factory=None):
        super(ReadOnly, self).__init__(default, factory)
        self.set_validate_kind(VALIDATE_READ_ONLY, None)


class Constant(Value):
    """ A value which cannot be changed from its default.

    """
    __slots__ = ()

    def __init__(self, default=None, factory=None):
        super(Constant, self).__init__(default, factory)
        self.set_validate_kind(VALIDATE_CONSTANT, None)


class Callable(Value):
    """ A value which is callable.

    """
    __slots__ = ()

    def __init__(self, default=None, factory=None):
        super(Callable, self).__init__(default, factory)
        self.set_validate_kind(VALIDATE_CALLABLE, None)


class Bool(Value):
    """ A value of type `bool`.

    """
    __slots__ = ()

    def __init__(self, default=False, factory=None):
        super(Bool, self).__init__(default, factory)
        self.set_validate_kind(VALIDATE_BOOL, None)


class Int(Value):
    """ A value of type `int`.

    """
    __slots__ = ()

    def __init__(self, default=0, factory=None):
        super(Int, self).__init__(default, factory)
        self.set_validate_kind(VALIDATE_INT, None)


class Long(Value):
    """ A value of type `long`.

    By default, ints are promoted to longs. Pass strict=True to the
    constructor to enable strict long checking.

    """
    __slots__ = ()

    def __init__(self, default=0L, factory=None, strict=False):
        super(Long, self).__init__(default, factory)
        if strict:
            self.set_validate_kind(VALIDATE_LONG, None)
        else:
            self.set_validate_kind(VALIDATE_LONG_PROMOTE, None)


class Range(Value):
    """ An integer value clipped to a range.

    """
    __slots__ = ()

    def __init__(self, low=None, high=None, value=None):
        if low is not None and high is not None and low > high:
            low, high = high, low
        default = 0
        if value is not None:
            default = value
        elif low is not None:
            default = low
        elif high is not None:
            default = high
        self.set_default_kind(DEFAULT_VALUE, default)
        self.set_validate_kind(VALIDATE_RANGE, (low, high))


class Float(Value):
    """ A value of type `float`.

    By default, ints and longs will be promoted to floats. Pass
    strict=True to the constructor to enable strict float checking.

    """
    __slots__ = ()

    def __init__(self, default=0.0, factory=None, strict=False):
        super(Float, self).__init__(default, factory)
        if strict:
            self.set_validate_kind(VALIDATE_FLOAT, None)
        else:
            self.set_validate_kind(VALIDATE_FLOAT_PROMOTE, None)


class Str(Value):
    """ A value of type `str`.

    """
    __slots__ = ()

    def __init__(self, default='', factory=None):
        super(Str, self).__init__(default, factory)
        self.set_validate_kind(VALIDATE_STR, None)


class Unicode(Value):
    """ A value of type `unicode`.

    By default, plain strings will be promoted to unicode strings. Pass
    strict=True to the constructor to enable strict unicode checking.

    """
    __slots__ = ()

    def __init__(self, default=u'', factory=None, strict=False):
        super(Unicode, self).__init__(default, factory)
        if strict:
            self.set_validate_kind(VALIDATE_UNICODE, None)
        else:
            self.set_validate_kind(VALIDATE_UNICODE_PROMOTE, None)


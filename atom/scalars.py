#------------------------------------------------------------------------------
#  Copyright (c) 2013, Enthought, Inc.
#  All rights reserved.
#------------------------------------------------------------------------------
from .catom import (
    Member, DEFAULT_FACTORY, DEFAULT_VALUE, VALIDATE_READ_ONLY,
    VALIDATE_CONSTANT, VALIDATE_CALLABLE, VALIDATE_BOOL, VALIDATE_INT,
    VALIDATE_LONG, VALIDATE_FLOAT, VALIDATE_STR, VALIDATE_UNICODE_PROMOTE
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


#------------------------------------------------------------------------------
#  Copyright (c) 2013, Enthought, Inc.
#  All rights reserved.
#------------------------------------------------------------------------------
from .catom import Member, DEFAULT_FACTORY, DEFAULT_VALUE, USER_VALIDATE


class Coerced(Member):
    """ A member which will coerce a value to a given kind.

    """
    __slots__ = ()

    def __init__(self, kind, factory=None, coercer=None):
        """ Initialize a Coerced.

        Parameters
        ----------
        kind : type or tuple of types
            The allowable types for the value.

        factory : callable, optional
            An optional callable which takes no arguments and returns
            the default value for the member. If this is not provided
            the default value will be None.

        coercer : callable, optional
            An optional callable which takes the value and returns the
            coerced value. If this is not given, then 'kind' must be
            a callable type which will be called to coerce the value.

        """
        if factory is not None:
            self.default_kind = (DEFAULT_FACTORY, factory)
        else:
            self.default_kind = (DEFAULT_VALUE, None)
        self.validate_kind = (USER_VALIDATE, (kind, coercer))

    def validate(self, owner, name, old, new):
        """ Validate the value of the member.

        If the value is an instance of the allowable types, it will be
        coerced to a value of the appropriate type.

        """
        kind, coercer = self.validate_kind[1]
        if isinstance(new, kind):
            return new
        coercer = coercer or kind
        try:
            res = coercer(new)
        except (TypeError, ValueError):
            raise TypeError('could not coerce value an appopriate type')
        return res


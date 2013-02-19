#------------------------------------------------------------------------------
#  Copyright (c) 2013, Enthought, Inc.
#  All rights reserved.
#------------------------------------------------------------------------------
from .catom import (
    Member, DEFAULT_FACTORY, USER_DEFAULT, VALIDATE_TYPED, USER_VALIDATE
)


class Typed(Member):
    """ A value which allows objects of a given type or types.

    Values will be tested using the `PyObject_TypeCheck` C API call.
    This call is equivalent to `type(obj) in cls.mro()`. It is less
    flexible but faster than Instance.

    A typed value may not be set to None. However, it may be set to
    null. This is chosen to more explicititly indicate the intent of
    Typed versus Instance.

    """
    __slots__ = ()

    def __init__(self, kind, factory=None, args=None, kwargs=None):
        """ Initialize an Instance.

        Parameters
        ----------
        kind : type
            The allowed type for the instance.

        factory : callable, optional
            An optional factory to use for creating the default value.
            If this is not provided and 'args' and 'kwargs' is None,
            then the default value will be null.

        args : tuple, optional
            If 'factory' is None, then 'kind' is a callable type and
            these arguments will be passed to the constructor to create
            the default value.

        kwargs : dict, optional
            If 'factory' is None, then 'kind' is a callable type and
            these keywords will be passed to the constructor to create
            the default value.

        """
        if factory is not None:
            self.set_default_kind(DEFAULT_FACTORY, factory)
        elif args is not None or kwargs is not None:
            args = args or ()
            kwargs = kwargs or {}
            factory = lambda: kind(*args, **kwargs)
            self.set_default_kind(DEFAULT_FACTORY, factory)
        self.set_validate_kind(VALIDATE_TYPED, kind)


class ForwardTyped(Typed):
    """ A Typed which delays resolving the type definition.

    The first time the value is accessed or modified, the type will
    be resolved and the forward instance will behave identically to
    a normal instance.

    """
    __slots__ = ()

    def __init__(self, resolve, factory=None, args=None, kwargs=None):
        """ Initialize a ForwardTyped.

        resolve : callable
            A callable which takes no arguments and returns the type to
            use for validating the values.

        factory : callable, optional
            An optional factory to use for creating the default value.
            If this is not provided and 'args' and 'kwargs' is None,
            then the default value will be None.

        args : tuple, optional
            If 'factory' is None, then 'resolve' will return a callable
            type and these arguments will be passed to the constructor
            to create the default value.

        kwargs : dict, optional
            If 'factory' is None, then 'resolve' will return a callable
            type and these keywords will be passed to the constructor to
            create the default value.

        """
        if factory is not None:
            self.set_default_kind(DEFAULT_FACTORY, factory)
        elif args is not None or kwargs is not None:
            args = args or ()
            kwargs = kwargs or {}
            self.set_default_kind(USER_DEFAULT, (args, kwargs))
        self.set_validate_kind(USER_VALIDATE, resolve)

    def default(self, owner, name):
        """ Called to retrieve the default value.

        This will resolve and instantiate the type. It will then update
        the internal default and validate handlers to behave like a
        normal instance member.

        """
        resolve = self.validate_kind[1]
        kind = resolve()
        args, kwargs = self.default_kind[1]
        value = kind(*args, **kwargs)
        self.set_default_kind(DEFAULT_FACTORY, lambda: kind(*args, **kwargs))
        self.set_validate_kind(VALIDATE_TYPED, kind)
        return value

    def validate(self, owner, name, old, new):
        """ Called to validate the value.

        This will resolve the type and validate the new value. It will
        then update the internal default and validate handlers to behave
        like a normal instance member.

        """
        resolve = self.validate_kind[1]
        kind = resolve()
        if type(new) not in kind.mro():
            raise TypeError('invalid type')
        self.set_validate_kind(VALIDATE_TYPED, kind)
        if self.default_kind[0] == USER_DEFAULT:
            args, kwargs = self.default_kind[1]
            factory = lambda: kind(*args, **kwargs)
            self.set_default_kind(DEFAULT_FACTORY, factory)
        return new


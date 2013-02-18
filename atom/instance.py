#------------------------------------------------------------------------------
#  Copyright (c) 2013, Enthought, Inc.
#  All rights reserved.
#------------------------------------------------------------------------------
from .catom import (
    Member, DEFAULT_FACTORY, DEFAULT_VALUE, USER_DEFAULT, VALIDATE_INSTANCE,
    USER_VALIDATE
)


class Instance(Member):
    """ A value which allows objects of a given type or types.

    Values will be tested using the `PyObject_IsInstance` C API call.
    This call is equivalent to `isinstance(value, kind)` and all the
    same rules apply.

    The value of an Instance may also be set to None.

    """
    __slots__ = ()

    def __init__(self, kind, factory=None, args=None, kwargs=None):
        """ Initialize an Instance.

        Parameters
        ----------
        kind : type or tuple of types
            The allowed type or types for the instance.

        factory : callable, optional
            An optional factory to use for creating the default value.
            If this is not provided and 'args' and 'kwargs' is None,
            then the default value will be None.

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
            self.default_kind = (DEFAULT_FACTORY, factory)
        elif args is not None or kwargs is not None:
            args = args or ()
            kwargs = kwargs or {}
            factory = lambda: kind(*args, **kwargs)
            self.default_kind = (DEFAULT_FACTORY, factory)
        else:
            self.default_kind = (DEFAULT_VALUE, None)
        self.validate_kind = (VALIDATE_INSTANCE, kind)


class ForwardInstance(Instance):
    """ An Instance which delays resolving the type definition.

    The first time the value is accessed or modified, the type will
    be resolved and the forward instance will behave identically to
    a normal instance.

    """
    __slots__ = ()

    def __init__(self, resolve, factory=None, args=None, kwargs=None):
        """ Initialize a ForwardInstance.

        resolve : callable
            A callable which takes no arguments and returns the type or
            tuple of types to use for validating the values.

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
            self.default_kind = (DEFAULT_FACTORY, factory)
        elif args is not None or kwargs is not None:
            args = args or ()
            kwargs = kwargs or {}
            self.default_kind = (USER_DEFAULT, (args, kwargs))
        else:
            self.default_kind = (DEFAULT_VALUE, None)
        self.validate_kind = (USER_VALIDATE, resolve)

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
        self.default_kind = (DEFAULT_FACTORY, lambda: kind(*args, **kwargs))
        self.validate_kind = (VALIDATE_INSTANCE, kind)
        return value

    def validate(self, owner, name, old, new):
        """ Called to validate the value.

        This will resolve the type and validate the new value. It will
        then update the internal default and validate handlers to behave
        like a normal instance member.

        """
        resolve = self.validate_kind[1]
        kind = resolve()
        if not isinstance(new, kind):
            raise TypeError('invalid instance type')
        self.validate_kind = (VALIDATE_INSTANCE, kind)
        if self.default_kind[0] == USER_DEFAULT:
            args, kwargs = self.default_kind[1]
            factory = lambda: kind(*args, **kwargs)
            self.default_kind = (DEFAULT_FACTORY, factory)
        return new


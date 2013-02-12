#------------------------------------------------------------------------------
#  Copyright (c) 2013, Enthought, Inc.
#  All rights reserved.
#------------------------------------------------------------------------------
from .catom import Member, null


class Value(Member):
    """ A simple member class which supports value initialization.

    A plain `Value` provides support for default values and factories,
    but does not perform any type checking or validation.

    """
    __slots__ = ('_default', '_factory')

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
        self.has_default = True
        self._default = default
        self._factory = factory

    def default(self, owner, name):
        """ Get the default value for the member.

        """
        factory = self._factory
        if factory is not None:
            return factory()
        return self._default


class ReadOnly(Value):
    """ A value which can be assigned once and is then read-only.

    """
    __slots__ = ()

    def __init__(self, default=None, factory=None):
        super(ReadOnly, self).__init__(default, factory)
        self.has_validate = True

    def validate(self, owner, name, old, new):
        if old is not null:
            raise RuntimeError("Cannot change a read-only value")
        return new


class Constant(Value):
    """ A value which cannot be changed from its default.

    """
    __slots__ = ()

    def __init__(self, default=None, factory=None):
        super(ReadOnly, self).__init__(default, factory)
        self.has_validate = True

    def validate(self, owner, name, old, new):
        raise RuntimeError("Cannot change a constant value")


class Typed(Value):
    """ A member class wich supports type validation.

    """
    __slots__ = ('_kind',)

    def __init__(self, kind=None, default=None, factory=None):
        """ Initialize a Typed member.

        Parameters
        ----------
        kind : type, optional
            The allowed type of values assigned to the member.

        default : object, optional
            The default value for the member.

        factory : callable, optional
            The default value factory for the member.

        """
        kind = kind or object
        assert isinstance(kind, type), "Kind must be a type"
        super(Typed, self).__init__(default, factory)
        self.has_validate = True
        self._kind = kind

    def validate(self, owner, name, old, new):
        """ Validate the value being assigned to the member.

        If the value is not valid, a TypeError is raised.

        Parameters
        ----------
        owner : Atom
            The atom object which owns the value being modified.

        name : str
            The member name of the atom being modified.

        old : object
            The old value of the member.

        new : object
            The value being assigned to the member.

        Returns
        -------
        result : object
            The original value, provided it passes type validation.

        """
        if not isinstance(new, self._kind):
            t = "The '%s' member on the `%s` object requires a value of type "
            t += "`%s`. Got value of type `%s` instead."
            owner_type = type(owner).__name__
            kind_type = self._kind.__name__
            value_type = type(new).__name__
            raise TypeError(t % (name, owner_type, kind_type, value_type))
        return new


class Bool(Typed):
    """ A typed member of type `bool`.

    """
    __slots__ = ()

    def __init__(self, default=False):
        assert isinstance(default, bool)
        super(Bool, self).__init__(bool, default)


class Int(Typed):
    """ A typed member of type `int`.

    """
    __slots__ = ()

    def __init__(self, default=0):
        assert isinstance(default, int)
        super(Int, self).__init__(int, default)


class Long(Typed):
    """ A typed member of type `long`.

    """
    __slots__ = ()

    def __init__(self, default=0L):
        assert isinstance(default, long)
        super(Long, self).__init__(long, default)


class Float(Typed):
    """ A typed member of type `float`.

    """
    __slots__ = ()

    def __init__(self, default=0.0):
        assert isinstance(default, float)
        super(Float, self).__init__(float, default)


class Str(Typed):
    """ A typed member of type `str`.

    """
    __slots__ = ()

    def __init__(self, default=''):
        assert isinstance(default, str)
        super(Str, self).__init__(str, default)


class Unicode(Typed):
    """ A typed member of type `unicode`.

    Regular strings will be promoted to unicode strings.

    """
    __slots__ = ()

    def __init__(self, default=u''):
        assert isinstance(default, unicode)
        super(Unicode, self).__init__(unicode, default)


class Tuple(Typed):
    """ A typed member of type `tuple`.

    """
    __slots__ = ()

    def __init__(self, default=()):
        assert isinstance(default, tuple)
        super(Tuple, self).__init__(tuple, default)


class List(Typed):
    """ A typed member of type `list`.

    """
    __slots__ = ()

    def __init__(self, default=None):
        if default is None:
            factory = list
        else:
            assert isinstance(default, list)
            factory = lambda: default[:]
        super(List, self).__init__(list, factory=factory)


class Dict(Typed):
    """ A typed member of type `dict`.

    """
    __slots__ = ()

    def __init__(self, default=None):
        if default is None:
            factory = dict
        else:
            assert isinstance(default, dict)
            factory = lambda: default.copy()
        super(Dict, self).__init__(dict, factory=factory)


class Instance(Typed):
    """ A typed member which allows the value to be set to None.

    """
    __slots__ = ()

    def validate(self, owner, name, old, new):
        if new is None:
            return new
        return super(Instance, self).validate(owner, name, old, new)


class Enum(Member):
    """ A member where the value can be one of a provided set.

    """
    __slots__ = ('_default', '_items')

    def __init__(self, *items, **kwargs):
        """ Initialize an Enum.

        *items
            The allowable items for the enum. There must be at least
            one and they must all be hashable. These items are shared
            amongst all Atom instances for class on which the Enum is
            defined, so mutable enum items should be avoided.

        **kwargs
            Additional keyword arguments for the enum:

                default : object
                    The default value to use for the enum. If this is
                    not given, the first of the given items is used.

        """
        if len(items) == 0:
            raise ValueError('an Enum requires at least 1 item')
        self.has_default = True
        self.has_validate = True
        self._items = set(items)
        if 'default' in kwargs:
            default = kwargs['default']
            if default not in self._items:
                t = 'default `%s` is not a valid enum item'
                raise ValueError(t % default)
        else:
            default = items[0]
        self._default = default

    def default(self, owner, name):
        """ Get the default value for the enum.

        """
        return self._default

    def validate(self, owner, name, old, new):
        """ Validate the enum value.

        """
        if new is null:
            return new
        if new not in self._items:
            raise ValueError('`%s` is not a valid enum item' % new)
        return new


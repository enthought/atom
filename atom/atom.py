#------------------------------------------------------------------------------
#  Copyright (c) 2013, Enthought, Inc.
#  All rights reserved.
#------------------------------------------------------------------------------
from contextlib import contextmanager
import re
from types import FunctionType

from .catom import CAtom, Member


def _clone_member(member):
    """ Make a clone of a member.

    This method will attempt to find a `clone` method on the member. In
    its absence, it will invoke the default constructor. No C++ data is
    copied from the old member to the new member.

    Parameters
    ----------
    member : Member
        The member to clone.

    Returns
    -------
    result : Member
        The cloned member.

    """
    if hasattr(member, 'clone'):
        clone = member.clone()
    else:
        clone = type(member)()
    return clone


class observe(object):
    """ A decorator which can be used to observe members on a class.

    class Foo(Atom)
        a = Member()
        b = Member()
        @observe('a|b', regex=True)
        def printer(self, change):
            print change

    """
    def __init__(self, name, regex=False):
        """ Initialize an observe decorator.

        Parameters
        ----------
        name : str
            The name or pattern string to use for matching names.

        regex : bool, optional
            If True, `name` is a regex pattern string to match against
            the class' member names. The default is False.

        """
        assert isinstance(name, str), "name must be a string"
        self.name = name
        self.regex = regex
        self.func = None
        self.func_name = None

    def __call__(self, func):
        """ Called to decorate the function.

        """
        assert isinstance(func, FunctionType), "func must be a function"
        self.func = func
        return self


class AtomMeta(type):
    """ The metaclass for classes derived from Atom.

    This metaclass computes the memory layout of the members in a given
    class, so that the CAtom class can allocate exactly enough space for
    the object data when it instantiates an object.

    All classes deriving from Atom will be automatically slotted, which
    will prevent the creation of an instance dictionary and also the
    ability of an Atom to be weakly referenceable. If that behavior is
    required, then a subclasss should declare the appropriate slots.

    """
    def __new__(meta, name, bases, dct):
        # Unless the developer requests slots, they are automatically
        # turned off. This prevents the creation of instance dicts and
        # other space consuming features unless explicitly requested.
        if '__slots__' not in dct:
            dct['__slots__'] = ()

        # Pass over the class dict once and collect the static observers.
        # The decorated versions swap the functions back into the dict.
        mangled = []
        decorated = []
        dec_seen = set()
        for key, value in dct.iteritems():
            if isinstance(value, observe):
                if value in dec_seen:
                    msg = 'cannot bind `observe` to multiple names'
                    raise TypeError(msg)
                dec_seen.add(value)
                decorated.append(value)
                value.func_name = key
                value = value.func
                dct[key] = value
            if key.startswith('_observe_') and isinstance(value, FunctionType):
                mangled.append(key)

        # Create the class object.
        cls = type.__new__(meta, name, bases, dct)

        # Walk the mro of the class, exluding itself, in reverse order
        # collecting all of the members into a single dict. The reverse
        # update preserves the mro of overridden members.
        members = {}
        for base in reversed(cls.__mro__[1:-1]):
            if base is not CAtom and issubclass(base, CAtom):
                members.update(base.__atom_members__)

        # Walk the dict a second time to collect the class members. This
        # assings the name and the index to the member. If a member is
        # overriding an existing member, the index of the old member is
        # reused and any static observers are copied over.
        these_members = set()
        for key, value in dct.iteritems():
            if isinstance(value, Member):
                if value in these_members:
                    raise TypeError('cannot bind member to multiple names')
                these_members.add(value)
                value._name = key
                if key in members:
                    supermember = members[key]
                    members[key] = value
                    value._index = supermember._index
                    value.copy_static_observers(supermember)
                else:
                    value._index = len(members)
                    members[key] = value

        # Add the mangled name static observers. If the matching member
        # is defined on a sublass, it must be cloned as to not effect
        # subclass instances.
        for mangled_name in mangled:
            target = mangled_name[9:]
            if target in members:
                member = members[target]
                if member not in these_members:
                    clone = _clone_member(member)
                    clone.copy_static_observers(member)
                    member = members[target] = clone
                    setattr(cls, target, member)
                member.add_static_observer(mangled_name)

        # Add the decorated static observers. If the matching member
        # is defined on a sublass, it must be cloned as to not effect
        # subclass instances.
        for ob in decorated:
            if not ob.regex:
                if ob.name in members:
                    member = members[ob.name]
                    if member not in these_members:
                        clone = _clone_member(member)
                        clone.copy_static_observers(member)
                        member = members[ob.name] = clone
                        setattr(cls, ob.name, member)
                    member.add_static_observer(ob.func_name)
            else:
                rgx = re.compile(ob.name)
                for key, member in members.iteritems():
                    if rgx.match(key):
                        if member not in these_members:
                            clone = _clone_member(member)
                            clone.copy_static_observers(member)
                            member = members[key] = clone
                            setattr(cls, key, member)
                        member.add_static_observer(ob.func_name)

        # Put a reference to the members dict on the class. This is used
        # by CAtom to query for the members and member count as needed.
        cls.__atom_members__ = members

        return cls


class Atom(CAtom):
    """ The base class for defining atom objects.

    `Atom` objects are special Python objects which never allocate an
    instance dictionary unless one is explicitly requested. The storage
    for an atom is instead computed from the `Member` objects declared
    on the class. Memory is reserved for these members with no over
    allocation.

    This restriction make atom objects a bit less flexible than normal
    Python objects, but they are between 3x-10x more memory efficient
    than normal objects depending on the number of attributes.

    """
    __metaclass__ = AtomMeta

    @classmethod
    def members(cls):
        """ Get the members dictionary for the type.

        Returns
        -------
        result : dict
            The dictionary of members defined on the class. User code
            should not modify the contents of the dict.

        """
        return cls.__atom_members__

    @contextmanager
    def suppress_notifications(self):
        """ Disable member notifications within in a context.

        Returns
        -------
        result : contextmanager
            A context manager which disables atom notifications for the
            duration of the context. When the context exits, the state
            is restored to its previous value.

        """
        old = self.set_notifications_enabled(False)
        yield
        self.set_notifications_enabled(old)


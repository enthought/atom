#------------------------------------------------------------------------------
#  Copyright (c) 2013, Enthought, Inc.
#  All rights reserved.
#------------------------------------------------------------------------------
from contextlib import contextmanager
import re
from types import FunctionType

from .catom import (
    CAtom, Member, DEFAULT_OWNER_METHOD, DEFAULT_VALUE, VALIDATE_OWNER_METHOD,
    POST_VALIDATE_OWNER_METHOD,
)


class observe(object):
    """ A decorator which can be used to observe members on a class.

    class Foo(Atom)
        a = Member()
        b = Member()
        @observe('a|b', regex=True)
        def printer(self, change):
            print change

    """
    __slots__ = ('name', 'regex', 'func', 'func_name')

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


class set_default(object):
    """ An object used to set the default value of a base class member.

    """
    __slots__ = ('value', 'name')

    def __init__(self, value):
        self.value = value
        self.name = None  # storage for the metaclass


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
        statics = []
        defaults = []
        validates = []
        decorated = []
        set_defaults = []
        post_validates = []
        dec_seen = set()
        for key, value in dct.iteritems():
            if isinstance(value, set_default):
                value.name = key
                set_defaults.append(value)
                continue
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
                statics.append(key)
            elif key.startswith('_default_') and isinstance(value, FunctionType):
                defaults.append(key)
            elif key.startswith('_validate_') and isinstance(value, FunctionType):
                validates.append(key)
            elif key.startswith('_post_validate_') and isinstance(value, FunctionType):
                post_validates.append(key)

        # Remove the set_default items before creating the class
        for sd in set_defaults:
            del dct[sd.name]

        # Create the class object.
        cls = type.__new__(meta, name, bases, dct)

        # Walk the mro of the class, exluding itself, in reverse order
        # collecting all of the members into a single dict. The reverse
        # update preserves the mro of overridden members.
        members = {}
        for base in reversed(cls.__mro__[1:-1]):
            if base is not CAtom and issubclass(base, CAtom):
                members.update(base.__atom_members__)

        # The set of members which belong to this class as opposed to
        # a base class. This enables the code which sets up the static
        # handlers to only clone when necessary.
        owned_members = set()

        # Resolve any conflicts with memory layout. Conflicts can occur
        # with multiple inheritance where the indices of multiple base
        # classes will overlap. When this happens, the members which
        # conflict must be cloned in order to occupy a new index.
        conflicts = []
        occupied = set()
        for member in members.itervalues():
            if member.index in occupied:
                conflicts.append(member)
            else:
                occupied.add(member.index)

        resolved_index = len(occupied)
        for member in conflicts:
            clone = member.clone()
            clone.set_member_index(resolved_index)
            owned_members.add(clone)
            members[clone.name] = clone
            setattr(cls, clone.name, clone)
            resolved_index += 1

        # Walk the set_default handlers and clone the base class member
        # with a new member with the appropriate default. Raise an error
        # if the set_default does not point to a member.
        for sd in set_defaults:
            if sd.name not in members:
                msg = "Invalid call to set_default(). '%s' is not a member "
                msg += "on the '%s' class."
                raise TypeError(msg  % (sd.name, name))
            member = members[sd.name]
            member = member.clone()
            owned_members.add(member)
            setattr(cls, sd.name, member)
            member.set_default_kind(DEFAULT_VALUE, sd.value)

        # Walk the dict a second time to collect the class members. This
        # assigns the name and the index to the member. If a member is
        # overriding an existing member, the memory index of the old
        # member is reused and any static observers are copied over.
        for key, value in dct.iteritems():
            if isinstance(value, Member):
                if value in owned_members:
                    raise TypeError('cannot bind member to multiple names')
                owned_members.add(value)
                value.set_member_name(key)
                if key in members:
                    supermember = members[key]
                    members[key] = value
                    value.set_member_index(supermember.index)
                    value.copy_static_observers(supermember)
                else:
                    value.set_member_index(len(members))
                    members[key] = value

        # Add the special statically defined behaviors for the members.
        # If the target member is defined on a subclass, it is clones
        # so that the behavior of the subclass is not modified.

        # Default value methods
        for mangled_name in defaults:
            target = mangled_name[9:]
            if target in members:
                member = members[target]
                if member not in owned_members:
                    member = member.clone()
                    members[target] = member
                    owned_members.add(member)
                    setattr(cls, target, member)
                member.set_default_kind(DEFAULT_OWNER_METHOD, mangled_name)

        # Validate methods
        for mangled_name in validates:
            target = mangled_name[10:]
            if target in members:
                member = members[target]
                if member not in owned_members:
                    member = member.clone()
                    members[target] = member
                    owned_members.add(member)
                    setattr(cls, target, member)
                member.set_validate_kind(VALIDATE_OWNER_METHOD, mangled_name)

        # Post validate methods
        for mangled_name in post_validates:
            target = mangled_name[15:]
            if target in members:
                member = members[target]
                if member not in owned_members:
                    member = member.clone()
                    members[target] = member
                    owned_members.add(member)
                    setattr(cls, target, member)
                member.set_post_validate_kind(POST_VALIDATE_OWNER_METHOD, mangled_name)

        # Static observers
        for mangled_name in statics:
            target = mangled_name[9:]
            if target in members:
                member = members[target]
                if member not in owned_members:
                    member = member.clone()
                    members[target] = member
                    owned_members.add(member)
                    setattr(cls, target, member)
                member.add_static_observer(mangled_name)

        # Decorated observers
        for ob in decorated:
            if not ob.regex:
                if ob.name in members:
                    member = members[ob.name]
                    if member not in owned_members:
                        member = member.clone()
                        members[ob.name] = member
                        owned_members.add(member)
                        setattr(cls, ob.name, member)
                    member.add_static_observer(ob.func_name)
            else:
                rgx = re.compile(ob.name)
                for key, member in members.iteritems():
                    if rgx.match(key):
                        if member not in owned_members:
                            member = member.clone()
                            members[key] = member
                            owned_members.add(member)
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


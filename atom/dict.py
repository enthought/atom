#------------------------------------------------------------------------------
#  Copyright (c) 2013, Enthought, Inc.
#  All rights reserved.
#------------------------------------------------------------------------------
from UserDict import DictMixin

from .catom import Member, null, DEFAULT_DICT, VALIDATE_DICT
from .instance import Instance


class Dict(Member):
    """ A value of type `dict`.

    """
    __slots__ = ()

    def __init__(self, key=None, value=None, default=None):
        """ Initialize a List.

        Parameters
        ----------
        kind : Member, type, or tuple of types, optional
            A member to use for validating the types of item allowed in
            the list. This can also be a type object or a tuple of types,
            in which case it will be wrapped with an Instance member. If
            this is not given, no item validation is performed.

        default : list, optional
            The default list of values. A new copy of this list will be
            created for each atom instance.

        """
        if key is not None:
            if not isinstance(key, Member):
                if isinstance(key, type):
                    key = Instance(key)
                else:
                    raise TypeError('bad key kind')
        if value is not None:
            if not isinstance(value, Member):
                if isinstance(value, type):
                    value = Instance(value)
                else:
                    raise TypeError('bad value kind')
        if default is not None:
            assert isinstance(default, dict), 'default must be a dict'
        self.set_default_kind(DEFAULT_DICT, default)
        self.set_validate_kind(VALIDATE_DICT, (key, value))

    def set_member_name(self, name):
        """ Assign the name to this member.

        This method is called by the Atom metaclass when a class is
        created. This makes sure the name of the internal members are
        also updated.

        """
        super(Dict, self).set_member_name(name)
        key, value = self.validate_kind[1]
        if key is not None:
            key.set_member_name(name + '|key')
        if value is not None:
            value.set_member_name(name + '|value')

    def set_member_index(self, index):
        """ Assign the index to this member.

        This method is called by the Atom metaclass when a class is
        created. This makes sure the index of the internal members are
        also updated.

        """
        super(Dict, self).set_member_index(index)
        key, value = self.validate_kind[1]
        if key is not None:
            key.set_member_index(index)
        if value is not None:
            value.set_member_index(index)

    def __get__(self, owner, cls):
        """ Get the dict object for the member.

        If validation is enabled for the dict, a dict proxy will be
        returned which will intercept method calls and perform the
        required validation.

        """
        # XXX move this down to C++ by wrapping the list in a proxy
        # during the validate method. Although, that will create a
        # reference cycle to the owner, which should be avoided. So
        # maybe wrapping on the fly is the better idea.
        if owner is None:
            return self
        data = super(Dict, self).__get__(owner, cls)
        key, value = self.validate_kind[1]
        if key is None and value is None:
            return data
        return _DictProxy(owner, key, value, data)


class _DictProxy(object, DictMixin):
    """ A private proxy object which validates dict modifications.

    Instances of this class should not be created by user code.

    """
    # XXX move this class down to C++
    def __init__(self, owner, keymember, valmember, data):
        self._owner = owner
        self._keymember = keymember
        self._valmember = valmember
        self._data = data

    def __getitem__(self, key):
        return self._data[key]

    def __setitem__(self, key, value):
        owner = self._owner
        if self._keymember is not None:
            key = self._keymember.do_validate(owner, null, key)
        if self._valmember is not None:
            value = self._valmember.do_validate(owner, null, value)
        self._data[key] = value

    def __delitem__(self, key):
        del self._data[key]

    def __iter__(self):
        return iter(self._data)

    def __contains__(self, key):
        return key in self._data

    def keys(self):
        return self._data.keys()

    def copy(self):
        return self._data.copy()

    def has_key(self, key):
        return key in self._data


#------------------------------------------------------------------------------
#  Copyright (c) 2013, Enthought, Inc.
#  All rights reserved.
#------------------------------------------------------------------------------
from .catom import Member, null, DEFAULT_LIST, VALIDATE_LIST
from .instance import Instance


class List(Member):
    """ A member which allows list values.

    Assigning to a list creates a copy. The orginal list will remain
    unmodified. This is similar to the semantics of the assignment
    operator on the C++ STL container classes.

    """
    __slots__ = '_member'

    def __init__(self, item=None, default=None):
        """ Initialize a List.

        Parameters
        ----------
        item : Member, type, or tuple of types, optional
            A member to use for validating the types of items allowed in
            the list. This can also be a type object or a tuple of types,
            in which case it will be wrapped with an Instance member. If
            this is not given, no item validation is performed.

        default : list, optional
            The default list of values. A new copy of this list will be
            created for each atom instance.

        """
        if item is not None:
            if not isinstance(item, Member):
                if isinstance(item, type):
                    item = Instance(item)
                else:
                    raise TypeError('bad List item')
        if default is not None:
            assert isinstance(default, list), 'default must be a list'
        self.set_default_kind(DEFAULT_LIST, default)
        self.set_validate_kind(VALIDATE_LIST, item)

    def set_member_name(self, name):
        """ Assign the name to this member.

        This method is called by the Atom metaclass when a class is
        created. This makes sure the name of the internal member is
        also updated.

        """
        super(List, self).set_member_name(name)
        member = self.validate_kind[1]
        if member is not None:
            member.set_member_name(name + "|item")

    def set_member_index(self, index):
        """ Assign the index to this member.

        This method is called by the Atom metaclass when a class is
        created. This makes sure the index of the internal member is
        also updated.

        """
        super(List, self).set_member_index(index)
        member = self.validate_kind[1]
        if member is not None:
            member.set_member_index(index)

    def __get__(self, owner, cls):
        """ Get the list object for the member.

        If validation is enabled for the list items, a list proxy will
        be returned which will intercept method calls and perform the
        required validation.

        """
        # XXX move this down to C++ by wrapping the list in a proxy
        # during the validate method. Although, that will create a
        # reference cycle to the owner, which should be avoided. So
        # maybe wrapping on the fly is the better idea.
        if owner is None:
            return self
        data = super(List, self).__get__(owner, cls)
        member = self.validate_kind[1]
        if member is None:
            return data
        return _ListProxy(owner, member, data)


class _ListProxy(object):
    """ A private proxy object which validates list modifications.

    Instances of this class should not be created by user code.

    """
    # XXX move this class down to C++
    def __init__(self, owner, member, data):
        self._owner = owner
        self._member = member
        self._data = data

    def __repr__(self):
        return repr(self._data)

    def __call__(self):
        return self._data

    def __iter__(self):
        return iter(self._data)

    def __getitem__(self, index):
        return self._data[index]

    def __setitem__(self, index, item):
        item = self._member.do_validate(self._owner, null, item)
        self._data[index] = item

    def __delitem__(self, index):
        del self._data[index]

    def append(self, item):
        item = self._member.do_validate(self._owner, null, item)
        self._data.append(item)

    def insert(self, index, item):
        item = self._member.do_validate(self._owner, null, item)
        self._data.insert(index, item)

    def extend(self, items):
        owner = self._owner
        validate = self._member.do_validate
        items = [validate(owner, null, item) for item in items]
        self._data.extend(items)

    def pop(self, *args):
        self._data.pop(*args)

    def remove(self, item):
        self._data.remove(item)

    def reverse(self):
        self._data.reverse()

    def sort(self, *args, **kwargs):
        self._data.sort(*args, **kwargs)


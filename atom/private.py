#------------------------------------------------------------------------------
#  Copyright (c) 2013, Enthought, Inc.
#  All rights reserved.
#------------------------------------------------------------------------------
import sys

from .catom import Member


class private(Member):

    __slots__ = ('member', 'func_codes')

    def __init__(self, member):
        self.member = member
        self.func_codes = ()

    def set_member_name(self, name):
        super(private, self).set_member_name(name)
        self.member.set_member_name(name)

    def set_member_index(self, index):
        super(private, self).set_member_index(index)
        self.member.set_member_index(index)

    def clone(self):
        clone = super(private, self).clone()
        clone.member = self.member.clone()
        clone.func_codes = self.func_codes
        return clone

    def set_default_kind(self, kind, context):
        self.member.set_default_kind(kind, context)

    def set_validate_kind(self, kind, context):
        self.member.set_validate_kind(kind, context)

    def __get__(self, owner, cls):
        if owner is None:
            return self
        f = sys._getframe(1)
        if f.f_code not in self.func_codes:
            raise TypeError("cannot access private attribute '%s'" % self.name)
        return self.member.__get__(owner, cls)

    def __set__(self, owner, value):
        f = sys._getframe(1)
        if f.f_code not in self.func_codes:
            raise TypeError("cannot access private attribute '%s'" % self.name)
        self.member.__set__(owner, value)


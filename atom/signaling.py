#------------------------------------------------------------------------------
#  Copyright (c) 2013, Enthought, Inc.
#  All rights reserved.
#------------------------------------------------------------------------------
from .catom import Member
from .signalbinder import SignalBinder


class Signal(Member):

    __slots__ = ()

    def __init__(self):
        self.has_default = True
        self.has_validate = True

    def default(self, owner, name):
        return SignalBinder()

    def validate(self, owner, name, old, new):
        raise TypeError('cannot assign to a signal')


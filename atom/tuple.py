#------------------------------------------------------------------------------
#  Copyright (c) 2013, Enthought, Inc.
#  All rights reserved.
#------------------------------------------------------------------------------
from .catom import Member, DEFAULT_FACTORY, DEFAULT_VALUE, VALIDATE_TUPLE


class Tuple(Member):
    """ A value of type `tuple`.

    """
    __slots__ = ()

    def __init__(self, default=(), factory=None):
        if factory is not None:
            self.set_default_kind(DEFAULT_FACTORY, factory)
        else:
            self.set_default_kind(DEFAULT_VALUE, default)
        self.set_validate_kind(VALIDATE_TUPLE, None)


#------------------------------------------------------------------------------
#  Copyright (c) 2013, Enthought, Inc.
#  All rights reserved.
#------------------------------------------------------------------------------
from .catom import Member, DEFAULT_DICT, VALIDATE_DICT


class Dict(Member):
    """ A value of type `dict`.

    """
    __slots__ = ()

    def __init__(self, default=None):
        self.default_kind = (DEFAULT_DICT, default)
        self.validate_kind = (VALIDATE_DICT, None)


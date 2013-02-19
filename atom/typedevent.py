#------------------------------------------------------------------------------
#  Copyright (c) 2013, Enthought, Inc.
#  All rights reserved.
#------------------------------------------------------------------------------
from .catom import Event, VALIDATE_TYPED


class TypedEvent(Event):
    """ An event which applies type checking to its argument.

    """
    __slots__ = ()

    def __init__(self, kind):
        """ Initialize a TypedEvent.

        Parameters
        ----------
        kind : type
            The type of argument which may be emitted by the event.

        """
        self.set_validate_kind(VALIDATE_TYPED, kind)


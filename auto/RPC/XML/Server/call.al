# NOTE: Derived from blib/lib/RPC/XML/Server.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package RPC::XML::Server;

#line 1661 "blib/lib/RPC/XML/Server.pm (autosplit into blib/lib/auto/RPC/XML/Server/call.al)"
###############################################################################
#
#   Sub Name:       call
#
#   Description:    This is an internal, end-run-around-dispatch() method to
#                   allow the RPC methods that this server has and knows about
#                   to call each other through their reference to the server
#                   object.
#
#   Arguments:      NAME      IN/OUT  TYPE      DESCRIPTION
#                   $self     in      ref       Object of this class
#                   $name     in      scalar    Name of the method to call
#                   @args     in      list      Arguments (if any) to pass
#
#   Returns:        Success:    return value of the call
#                   Failure:    error string
#
###############################################################################
sub call
{
    my ($self, $name, @args) = @_;

    my $meth;

    #
    # Two VERY important notes here: The values in @args are not pre-treated
    # in any way, so not only should the receiver understand what they're
    # getting, there's no signature checking taking place, either.
    #
    # Second, if the normal return value is not distinguishable from a string,
    # then the caller may not recognize if an error occurs.
    #

    return $meth unless ref($meth = $self->get_method($name));
    $meth->call($self, @args);
}

# end of RPC::XML::Server::call
1;

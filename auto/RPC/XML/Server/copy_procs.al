# NOTE: Derived from blib/lib/RPC/XML/Server.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package RPC::XML::Server;

#line 2012 "blib/lib/RPC/XML/Server.pm (autosplit into blib/lib/auto/RPC/XML/Server/copy_procs.al)"
# Same as above, but for name-symmetry
sub copy_procs { shift->copy_methods(@_) }

###############################################################################
#
#   Sub Name:       timeout
#
#   Description:    This sets the timeout for processing connections after
#                   a new connection has been accepted.  It returns the old
#                   timeout value.  If you pass in no value, it returns
#                   the current timeout.
#
#   Arguments:      NAME      IN/OUT  TYPE      DESCRIPTION
#                   $self     in      ref       Object reference/static class
#                   $timeout  in      ref       New timeout value
#
#   Returns:        $self->{__timeout}
#
###############################################################################
# end of RPC::XML::Server::copy_procs
1;

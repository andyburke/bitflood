# NOTE: Derived from blib/lib/RPC/XML/Server.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package RPC::XML::Server;

#line 1172 "blib/lib/RPC/XML/Server.pm (autosplit into blib/lib/auto/RPC/XML/Server/get_proc.al)"
# Same as above, but for name-symmetry
sub get_proc { shift->get_method(@_) }

###############################################################################
#
#   Sub Name:       server_loop
#
#   Description:    Enter a server-loop situation, using the accept() loop of
#                   HTTP::Daemon if $self has such an object, or falling back
#                   Net::Server otherwise.
#
#   Arguments:      NAME      IN/OUT  TYPE      DESCRIPTION
#                   $self     in      ref       Object of this class
#                   %args     in      hash      Additional parameters to set up
#                                                 before calling the superclass
#                                                 Run method
#
#   Returns:        string if error, otherwise void
#
###############################################################################
# end of RPC::XML::Server::get_proc
1;

# NOTE: Derived from blib/lib/RPC/XML/Server.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package RPC::XML::Server;

#line 1836 "blib/lib/RPC/XML/Server.pm (autosplit into blib/lib/auto/RPC/XML/Server/list_procs.al)"
# Same as above, but for name-symmetry
sub list_procs { shift->list_methods(@_) }

###############################################################################
#
#   Sub Name:       share_methods
#
#   Description:    Share the named methods as found on $src_srv into the
#                   method table of the calling object.
#
#   Arguments:      NAME      IN/OUT  TYPE      DESCRIPTION
#                   $self     in      ref       Object of this class
#                   $src_srv  in      ref       Another object of this class
#                   @names    in      list      One or more method names
#
#   Returns:        Success:    $self
#                   Failure:    error message
#
###############################################################################
# end of RPC::XML::Server::list_procs
1;

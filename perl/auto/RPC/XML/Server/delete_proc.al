# NOTE: Derived from blib/lib/RPC/XML/Server.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package RPC::XML::Server;

#line 1815 "blib/lib/RPC/XML/Server.pm (autosplit into blib/lib/auto/RPC/XML/Server/delete_proc.al)"
# Same as above, but for name-symmetry
sub delete_proc { shift->delete_method(@_) }

###############################################################################
#
#   Sub Name:       list_methods
#
#   Description:    Return a list of the methods this object has published.
#                   Returns the names, not the objects.
#
#   Arguments:      NAME      IN/OUT  TYPE      DESCRIPTION
#                   $self     in      ref       Object of this class
#
#   Returns:        List of names, possibly empty
#
###############################################################################
# end of RPC::XML::Server::delete_proc
1;

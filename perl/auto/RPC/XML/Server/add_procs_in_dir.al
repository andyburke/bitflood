# NOTE: Derived from blib/lib/RPC/XML/Server.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package RPC::XML::Server;

#line 1776 "blib/lib/RPC/XML/Server.pm (autosplit into blib/lib/auto/RPC/XML/Server/add_procs_in_dir.al)"
# Same as above, but for name-symmetry
sub add_procs_in_dir { shift->add_methods_in_dir(@_) }

###############################################################################
#
#   Sub Name:       delete_method
#
#   Description:    Remove any current binding for the named method on the
#                   calling server object. Note that if this method is shared
#                   across other server objects, it won't be destroyed until
#                   the last server deletes it.
#
#   Arguments:      NAME      IN/OUT  TYPE      DESCRIPTION
#                   $self     in      ref       Object of this class
#                   $name     in      scalar    Name of method to lost
#
#   Returns:        Success:    $self
#                   Failure:    error message
#
###############################################################################
# end of RPC::XML::Server::add_procs_in_dir
1;

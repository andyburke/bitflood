# NOTE: Derived from blib/lib/RPC/XML/Server.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package RPC::XML::Server;

#line 1122 "blib/lib/RPC/XML/Server.pm (autosplit into blib/lib/auto/RPC/XML/Server/proc_from_file.al)"
# Same as above, but for name-symmetry
sub proc_from_file { shift->method_from_file(@_) }

###############################################################################
#
#   Sub Name:       get_method
#
#   Description:    Get the current binding for the remote-side method $name.
#                   Returns undef if the method is not defined for the server
#                   instance.
#
#   Arguments:      NAME      IN/OUT  TYPE      DESCRIPTION
#                   $self     in      ref       Class instance
#                   $name     in      scalar    Name of the method being looked
#                                                 up
#
#   Returns:        Success:    Method-class reference
#                   Failure:    error string
#
###############################################################################
# end of RPC::XML::Server::proc_from_file
1;

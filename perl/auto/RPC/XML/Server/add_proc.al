# NOTE: Derived from blib/lib/RPC/XML/Server.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package RPC::XML::Server;

#line 1058 "blib/lib/RPC/XML/Server.pm (autosplit into blib/lib/auto/RPC/XML/Server/add_proc.al)"
###############################################################################
#
#   Sub Name:       add_proc
#
#   Description:    This filters through to add_method, but unlike the other
#                   front-ends defined later, this one may have to alter the
#                   data in one type of calling-convention.
#
#   Arguments:      NAME      IN/OUT  TYPE      DESCRIPTION
#                   $self     in      ref       Object reference
#                   $meth     in      scalar    Procedure to add
#
#   Returns:        threads through to add_method
#
###############################################################################
sub add_proc
{
    my ($self, $meth) = @_;

    # Anything else but a hash-reference goes through unaltered
    $meth->{type} = 'procedure' if (ref($meth) eq 'HASH');

    $self->add_method($meth);
}

# end of RPC::XML::Server::add_proc
1;

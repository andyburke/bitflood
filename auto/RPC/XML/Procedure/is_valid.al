# NOTE: Derived from blib/lib/RPC/XML/Procedure.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package RPC::XML::Procedure;

#line 635 "blib/lib/RPC/XML/Procedure.pm (autosplit into blib/lib/auto/RPC/XML/Procedure/is_valid.al)"
###############################################################################
#
#   Sub Name:       is_valid
#
#   Description:    Boolean test to tell if the calling object has sufficient
#                   data to be used as a server method for RPC::XML::Server or
#                   Apache::RPC::Server.
#
#   Arguments:      NAME      IN/OUT  TYPE      DESCRIPTION
#                   $self     in      ref       Object to test
#
#   Returns:        Success:    1, valid/complete
#                   Failure:    0, invalid/incomplete
#
###############################################################################
sub is_valid
{
    my $self = shift;

    return ((ref($self->{code}) eq 'CODE') and $self->{name} and
            (ref($self->{signature}) && scalar(@{$self->{signature}})));
}

# end of RPC::XML::Procedure::is_valid
1;

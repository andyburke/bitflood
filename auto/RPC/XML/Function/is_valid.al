# NOTE: Derived from blib/lib/RPC/XML/Function.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package RPC::XML::Function;

#line 263 "blib/lib/RPC/XML/Function.pm (autosplit into blib/lib/auto/RPC/XML/Function/is_valid.al)"
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

    return ((ref($self->{code}) eq 'CODE') and $self->{name});
}

# end of RPC::XML::Function::is_valid
1;

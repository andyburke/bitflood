# NOTE: Derived from blib/lib/RPC/XML/Procedure.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package RPC::XML::Procedure;

#line 732 "blib/lib/RPC/XML/Procedure.pm (autosplit into blib/lib/auto/RPC/XML/Procedure/match_signature.al)"
###############################################################################
#
#   Sub Name:       match_signature
#
#   Description:    Determine if the passed-in signature string matches any
#                   of this method's known signatures.
#
#   Arguments:      NAME      IN/OUT  TYPE      DESCRIPTION
#                   $self     in      ref       Object of this class
#                   $sig      in      scalar    Signature to check for
#
#   Returns:        Success:    return type as a string
#                   Failure:    0
#
###############################################################################
sub match_signature
{
    my $self = shift;
    my $sig  = shift;

    $sig = join(' ', @$sig) if ref $sig;

    return $self->{sig_table}->{$sig} || 0;
}

# end of RPC::XML::Procedure::match_signature
1;

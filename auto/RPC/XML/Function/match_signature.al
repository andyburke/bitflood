# NOTE: Derived from blib/lib/RPC/XML/Function.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package RPC::XML::Function;

#line 285 "blib/lib/RPC/XML/Function.pm (autosplit into blib/lib/auto/RPC/XML/Function/match_signature.al)"
###############################################################################
#
#   Sub Name:       match_signature
#
#   Description:    Noop. Needed for RPC::XML::Server.
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
    'scalar';
}

1;
# end of RPC::XML::Function::match_signature

# NOTE: Derived from blib/lib/RPC/XML/Procedure.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package RPC::XML::Procedure;

#line 883 "blib/lib/RPC/XML/Procedure.pm (autosplit into blib/lib/auto/RPC/XML/Procedure/call.al)"
###############################################################################
#
#   Sub Name:       call
#
#   Description:    Encapsulates the invocation of the code block that the
#                   object is abstracting. Manages parameters, signature
#                   checking, etc.
#
#   Arguments:      NAME      IN/OUT  TYPE      DESCRIPTION
#                   $self     in      ref       Object of this class
#                   $srv      in      ref       An object derived from the
#                                                 RPC::XML::Server class
#                   @dafa     in      list      The params for the call itself
#
#   Globals:        None.
#
#   Environment:    None.
#
#   Returns:        Success:    value
#                   Failure:    dies with RPC::XML::Fault object as message
#
###############################################################################
sub call
{
    my ($self, $srv, @data) = @_;

    my (@paramtypes, @params, $signature, $resptype, $response, $name);

    $name = $self->name;
    # Create the param list.
    # The type for the response will be derived from the matching signature
    @paramtypes = map { $_->type  } @data;
    @params     = map { $_->value } @data;
    $signature = join(' ', @paramtypes);
    $resptype = $self->match_signature($signature);
    # Since there must be at least one signature with a return value (even
    # if the param list is empty), this tells us if the signature matches:
    return RPC::XML::fault->new(301,
                                "method $name has no matching " .
                                'signature for the argument list: ' .
                                "[$signature]")
        unless ($resptype);

    # Set these in case the server object is part of the param list
    local $srv->{signature} = [ $resptype, @paramtypes ];
    local $srv->{method_name} = $name;
    # For RPC::XML::Method (and derivatives), pass the server object
    if ($self->isa('RPC::XML::Method'))
    {
        unshift(@params, $srv);
    }

    # Now take a deep breath and call the method with the arguments
    eval { $response = $self->{code}->(@params); };
    # Report a Perl-level error/failure if it occurs
    return RPC::XML::fault->new(302, "Method $name returned error: $@") if $@;

    $self->{called}++;
    # Create a suitable return value
    if ((! ref($response)) && UNIVERSAL::can("RPC::XML::$resptype", 'new'))
    {
        my $class = "RPC::XML::$resptype";
        $response = $class->new($response);
    }

    $response;
}

1;
# end of RPC::XML::Procedure::call

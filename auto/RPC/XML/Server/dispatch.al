# NOTE: Derived from blib/lib/RPC/XML/Server.pm.
# Changes made here will be lost when autosplit is run again.
# See AutoSplit.pm.
package RPC::XML::Server;

#line 1590 "blib/lib/RPC/XML/Server.pm (autosplit into blib/lib/auto/RPC/XML/Server/dispatch.al)"
###############################################################################
#
#   Sub Name:       dispatch
#
#   Description:    Route the request by parsing it, determining what the
#                   Perl routine should be, etc.
#
#   Arguments:      NAME      IN/OUT  TYPE      DESCRIPTION
#                   $self     in      ref       Object of this class
#                   $xml      in      ref       Reference to the XML text, or
#                                                 a RPC::XML::request object.
#                                                 If it is a listref, assume
#                                                 [ name, @args ].
#                   $reftable in      hashref   If present, a reference to the
#                                                 current-running table of
#                                                 back-references
#
#   Returns:        RPC::XML::response object
#
###############################################################################
sub dispatch
{
    my ($self, $xml) = @_;

    my ($reqobj, @data, $response, $name, $meth);

    if (ref($xml) eq 'SCALAR')
    {
        $reqobj = $self->parser->parse($$xml);
        return RPC::XML::response
            ->new(RPC::XML::fault->new(200, "XML parse failure: $reqobj"))
                unless (ref $reqobj);
    }
    elsif (ref($xml) eq 'ARRAY')
    {
        # This is sort of a cheat, to make the system.multicall API call a
        # lot easier. The syntax isn't documented in the manual page, for good
        # reason.
        $reqobj = RPC::XML::request->new(shift(@$xml), @$xml);
    }
    elsif (UNIVERSAL::isa($xml, 'RPC::XML::request'))
    {
        $reqobj = $xml;
    }
    else
    {
        $reqobj = $self->parser->parse($xml);
        return RPC::XML::response
            ->new(RPC::XML::fault->new(200, "XML parse failure: $reqobj"))
                unless (ref $reqobj);
    }

    @data = @{$reqobj->args};
    $name = $reqobj->name;

    # Get the method, call it, and bump the internal requests counter. Create
    # a fault object if there is problem with the method object itself.
    if (ref($meth = $self->get_method($name)))
    {
        $response = $meth->call($self, @data);
        $self->{__requests}++;
    }
    else
    {
        $response = RPC::XML::fault->new(300, $meth);
    }

    # All the eval'ing and error-trapping happened within the method class
    RPC::XML::response->new($response);
}

# end of RPC::XML::Server::dispatch
1;

<?xml version="1.0"?>
<!DOCTYPE methoddef SYSTEM "rpc-method.dtd">
<!--
    Generated automatically by make_method v1.09, Wed Aug  4 21:45:10 2004

    Any changes made here will be lost.
-->
<methoddef>
<name>system.multicall</name>
<version>1.0</version>
<signature>array array</signature>
<help>
Execute a set of one or more procedure calls on the server as a single
request. The only supported call signature takes an ARRAY of STRUCT values.
Each STRUCT should have two members:

    methodName         The name of the method/routine to invoke as a STRING
    params             An ARRAY of the parameters to pass to the routine

If the "params" member is absent, a call with no parameters is assumed. The
ARRAY of parameters will be expanded prior to the call, otherwise all the
called routines would have to have a signature allowing for a single ARRAY
input. Thus, any routine taking such an input will have to nest it within an
outer containing ARRAY.

The return value is an ARRAY of the return values from the calls, or a fault
response if one of the calls failed. Because the specification does not allow
for faults as first-class datatypes, all other results are discarded upon an
error, and any remaining calls will not get executed.
</help>
<code language="perl">
<![CDATA[
#!/usr/bin/perl
###############################################################################
#
#   Sub Name:       multicall
#
#   Description:    Execute multiple method calls in a single request
#
#   Arguments:      NAME      IN/OUT  TYPE      DESCRIPTION
#                   $srv      in      ref       Server object instance
#                   $list     in      ref       List of struct's with the call
#                                                 data within.
#
#   Globals:        None.
#
#   Environment:    None.
#
#   Returns:        Success:    listref
#                   Failure:    fault object
#
###############################################################################
sub multicall
{
    use strict;

    my $srv = shift;
    my $list = shift;

    my ($call, $subname, $params, $result, @results);

    my $name = $srv->{method_name};

    for $call (@$list)
    {
        unless (ref($call) eq 'HASH')
        {
            return RPC::XML::fault->new(200,
                                        "$name: One of the array elements " .
                                        'passed in was not a struct');
        }

        return RPC::XML::fault->new(310,
                                    "$name: Request was missing required " .
                                    '"methodName" member')
            unless ($subname = $call->{methodName});
        return RPC::XML::fault->new(310,
                                    "$name: Recursive calling of $name not " .
                                    'allowed')
            if ($subname eq $name);

        $params = $call->{params} || [];
        return RPC::XML::fault->new(200,
                                    "$name: Request's value for \"params\" " .
                                    'was not an array')
            unless (ref($params) eq 'ARRAY');

        $result = $srv->dispatch([ $subname, @$params ]);
        return $result if $result->is_fault;

        push @results, $result->value;
    }

    \@results;
}

__END__
]]></code>
</methoddef>

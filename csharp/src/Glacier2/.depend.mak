
Metrics.cs: \
    "$(slicedir)\Glacier2\Metrics.ice" \
    "$(slicedir)/Ice/Metrics.ice" \
    "$(slicedir)/Ice/BuiltinSequences.ice"

PermissionsVerifier.cs: \
    "$(slicedir)\Glacier2\PermissionsVerifier.ice" \
    "$(slicedir)/Glacier2/SSLInfo.ice" \
    "$(slicedir)/Ice/BuiltinSequences.ice"

Router.cs: \
    "$(slicedir)\Glacier2\Router.ice" \
    "$(slicedir)/Ice/Router.ice" \
    "$(slicedir)/Ice/BuiltinSequences.ice" \
    "$(slicedir)/Glacier2/Session.ice" \
    "$(slicedir)/Ice/Identity.ice" \
    "$(slicedir)/Glacier2/SSLInfo.ice" \
    "$(slicedir)/Glacier2/PermissionsVerifier.ice"

Session.cs: \
    "$(slicedir)\Glacier2\Session.ice" \
    "$(slicedir)/Ice/BuiltinSequences.ice" \
    "$(slicedir)/Ice/Identity.ice" \
    "$(slicedir)/Glacier2/SSLInfo.ice"

SSLInfo.cs: \
    "$(slicedir)\Glacier2\SSLInfo.ice" \
    "$(slicedir)/Ice/BuiltinSequences.ice"

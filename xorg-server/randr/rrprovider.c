/*
 * Copyright © 2012 Red Hat Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 *
 * Authors: Dave Airlie
 */

#include "randrstr.h"
#include "swaprep.h"

RESTYPE RRProviderType;

/*
 * Initialize provider type error value
 */
void
RRProviderInitErrorValue(void)
{
    SetResourceTypeErrorValue(RRProviderType, RRErrorBase + BadRRProvider);
}

#define ADD_PROVIDER(_pScreen) do {                                 \
    pScrPriv = rrGetScrPriv((_pScreen));                            \
    if (pScrPriv->provider) {                                   \
        providers[count_providers] = pScrPriv->provider->id;    \
        if (client->swapped)                                    \
            swapl(&providers[count_providers]);                 \
        count_providers++;                                      \
    }                                                           \
    } while(0)

int
ProcRRGetProviders (ClientPtr client)
{
    REQUEST(xRRGetProvidersReq);
    xRRGetProvidersReply rep;
    WindowPtr pWin;
    ScreenPtr pScreen;
    rrScrPrivPtr pScrPriv;
    int rc;
    CARD8 *extra;
    unsigned int extraLen;
    RRProvider *providers;
    int total_providers = 0, count_providers = 0;
    ScreenPtr iter;

    REQUEST_SIZE_MATCH(xRRGetProvidersReq);
    rc = dixLookupWindow(&pWin, stuff->window, client, DixGetAttrAccess);
    if (rc != Success)
        return rc;

    pScreen = pWin->drawable.pScreen;

    pScrPriv = rrGetScrPriv(pScreen);

    if (pScrPriv->provider)
        total_providers++;
    xorg_list_for_each_entry(iter, &pScreen->output_slave_list, output_head) {
        pScrPriv = rrGetScrPriv(iter);
        total_providers += pScrPriv->provider ? 1 : 0;
    }
    xorg_list_for_each_entry(iter, &pScreen->offload_slave_list, offload_head) {
        pScrPriv = rrGetScrPriv(iter);
        total_providers += pScrPriv->provider ? 1 : 0;
    }
    xorg_list_for_each_entry(iter, &pScreen->unattached_list, unattached_head) {
        pScrPriv = rrGetScrPriv(iter);
        total_providers += pScrPriv->provider ? 1 : 0;
    }

    pScrPriv = rrGetScrPriv(pScreen);

    if (!pScrPriv)
    {
 
        rep.type = X_Reply;
        rep.sequenceNumber = client->sequence;
        rep.length = 0;
        rep.timestamp = currentTime.milliseconds;
        rep.nProviders = 0;

        extra = NULL;
        extraLen = 0;
    } else {

        rep.type = X_Reply;
        rep.sequenceNumber = client->sequence;
        rep.timestamp = pScrPriv->lastSetTime.milliseconds;
        rep.nProviders = total_providers;
        rep.length = total_providers;

        extraLen = rep.length << 2;
        if (extraLen) {
            extra = malloc(extraLen);
            if (!extra)
                return BadAlloc;
        } else
            extra = NULL;

        providers = (RRProvider *)extra;
        ADD_PROVIDER(pScreen);
        xorg_list_for_each_entry(iter, &pScreen->output_slave_list, output_head) {
            ADD_PROVIDER(iter);
        }
        xorg_list_for_each_entry(iter, &pScreen->offload_slave_list, offload_head) {
            ADD_PROVIDER(iter);
        }
        xorg_list_for_each_entry(iter, &pScreen->unattached_list, unattached_head) {
            ADD_PROVIDER(iter);
        }
    }

    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.timestamp);
        swaps(&rep.nProviders);
    }
    WriteToClient(client, sizeof(xRRGetProvidersReply), (char *)&rep);
    if (extraLen)
    {
        WriteToClient (client, extraLen, (char *) extra);
        free(extra);
    }
    return Success;
}

int
ProcRRGetProviderInfo (ClientPtr client)
{
    REQUEST(xRRGetProviderInfoReq);
    xRRGetProviderInfoReply rep;
    rrScrPrivPtr pScrPriv, pScrProvPriv;
    RRProviderPtr provider;
    ScreenPtr pScreen;
    CARD8 *extra;
    unsigned int extraLen = 0;
    RRCrtc *crtcs;
    RROutput *outputs;
    int i;
    char *name;
    ScreenPtr provscreen;
    RRProvider *providers;
    uint32_t *prov_cap;
 
    REQUEST_SIZE_MATCH(xRRGetProviderInfoReq);
    VERIFY_RR_PROVIDER(stuff->provider, provider, DixReadAccess);

    pScreen = provider->pScreen;
    pScrPriv = rrGetScrPriv(pScreen);


    rep.type = X_Reply;
    rep.status = RRSetConfigSuccess;
    rep.sequenceNumber = client->sequence;
    rep.length = 0;
    rep.capabilities = provider->capabilities;
    rep.nameLength = provider->nameLength;
    rep.timestamp = pScrPriv->lastSetTime.milliseconds;
    rep.nCrtcs = pScrPriv->numCrtcs;
    rep.nOutputs = pScrPriv->numOutputs;
    rep.nAssociatedProviders = 0;


    /* count associated providers */
    if (provider->offload_sink)
        rep.nAssociatedProviders++;
    if (provider->output_source)
        rep.nAssociatedProviders++;
    xorg_list_for_each_entry(provscreen, &pScreen->output_slave_list, output_head)
        rep.nAssociatedProviders++;
    xorg_list_for_each_entry(provscreen, &pScreen->offload_slave_list, offload_head)
        rep.nAssociatedProviders++;

    rep.length = (pScrPriv->numCrtcs + pScrPriv->numOutputs +
                  (rep.nAssociatedProviders * 2) + bytes_to_int32(rep.nameLength));

    extraLen = rep.length << 2;
    if (extraLen) {
        extra = malloc(extraLen);
        if (!extra)
            return BadAlloc;
    }
    else
        extra = NULL;

    crtcs = (RRCrtc *)extra;
    outputs = (RROutput *)(crtcs + rep.nCrtcs);
    providers = (RRProvider *)(outputs + rep.nOutputs);
    prov_cap = (unsigned int *)(providers + rep.nAssociatedProviders);
    name = (char *)(prov_cap + rep.nAssociatedProviders);

    for (i = 0; i < pScrPriv->numCrtcs; i++) {
        crtcs[i] = pScrPriv->crtcs[i]->id;
        if (client->swapped)
            swapl(&crtcs[i]);
    }

    for (i = 0; i < pScrPriv->numOutputs; i++) {
        outputs[i] = pScrPriv->outputs[i]->id;
        if (client->swapped)
            swapl(&outputs[i]);
    }

    i = 0;
    if (provider->offload_sink) {
        providers[i] = provider->offload_sink->id;
        if (client->swapped)
            swapl(&providers[i]);
        prov_cap[i] = RR_Capability_SinkOffload;
        if (client->swapped)
            swapl(&prov_cap[i]);
        i++;
    }
    if (provider->output_source) {
        providers[i] = provider->output_source->id;
        if (client->swapped)
            swapl(&providers[i]);
        prov_cap[i] = RR_Capability_SourceOutput;
            swapl(&prov_cap[i]);
        i++;
    }
    xorg_list_for_each_entry(provscreen, &pScreen->output_slave_list, output_head) {
        pScrProvPriv = rrGetScrPriv(provscreen);
        providers[i] = pScrProvPriv->provider->id;
        if (client->swapped)
            swapl(&providers[i]);
        prov_cap[i] = RR_Capability_SinkOutput;
        if (client->swapped)
            swapl(&prov_cap[i]);
        i++;
    }
    xorg_list_for_each_entry(provscreen, &pScreen->offload_slave_list, offload_head) {
        pScrProvPriv = rrGetScrPriv(provscreen);
        providers[i] = pScrProvPriv->provider->id;
        if (client->swapped)
            swapl(&providers[i]);
        prov_cap[i] = RR_Capability_SourceOffload;
        if (client->swapped)
            swapl(&prov_cap[i]);
        i++;
    }


    memcpy(name, provider->name, rep.nameLength);
    if (client->swapped) {
              swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.capabilities);
        swaps(&rep.nCrtcs);
        swaps(&rep.nOutputs);
        swaps(&rep.nameLength);
    }
    WriteToClient(client, sizeof(xRRGetProviderInfoReply), (char *)&rep);
    if (extraLen)
    {
        WriteToClient (client, extraLen, (char *) extra);
        free(extra);
    }
    return Success;
}

int
ProcRRSetProviderOutputSource(ClientPtr client)
{
    REQUEST(xRRSetProviderOutputSourceReq);
    rrScrPrivPtr pScrPriv;
    RRProviderPtr provider, source_provider = NULL;
    ScreenPtr pScreen;

    REQUEST_AT_LEAST_SIZE(xRRSetProviderOutputSourceReq);

    VERIFY_RR_PROVIDER(stuff->provider, provider, DixReadAccess);

    if (!(provider->capabilities & RR_Capability_SinkOutput))
        return BadValue;

    if (stuff->source_provider) {
        VERIFY_RR_PROVIDER(stuff->source_provider, source_provider, DixReadAccess);

        if (!(source_provider->capabilities & RR_Capability_SourceOutput))
            return BadValue;
    }

    pScreen = provider->pScreen;
    pScrPriv = rrGetScrPriv(pScreen);

    pScrPriv->rrProviderSetOutputSource(pScreen, provider, source_provider);

    provider->changed = TRUE;
    RRSetChanged(pScreen);

    RRTellChanged (pScreen);

    return Success;
}

int
ProcRRSetProviderOffloadSink(ClientPtr client)
{
    REQUEST(xRRSetProviderOffloadSinkReq);
    rrScrPrivPtr pScrPriv;
    RRProviderPtr provider, sink_provider = NULL;
    ScreenPtr pScreen;

    REQUEST_AT_LEAST_SIZE(xRRSetProviderOffloadSinkReq);

    VERIFY_RR_PROVIDER(stuff->provider, provider, DixReadAccess);
    if (!(provider->capabilities & RR_Capability_SourceOffload))
        return BadValue;

    if (stuff->sink_provider) {
        VERIFY_RR_PROVIDER(stuff->sink_provider, sink_provider, DixReadAccess);
        if (!(sink_provider->capabilities & RR_Capability_SinkOffload))
            return BadValue;
    }
    pScreen = provider->pScreen;
    pScrPriv = rrGetScrPriv(pScreen);

    pScrPriv->rrProviderSetOffloadSink(pScreen, provider, sink_provider);

    provider->changed = TRUE;
    RRSetChanged(pScreen);

    RRTellChanged (pScreen);

    return Success;
}

RRProviderPtr
RRProviderCreate(ScreenPtr pScreen, const char *name,
                 int nameLength)
{
    RRProviderPtr provider;
    rrScrPrivPtr pScrPriv;

    pScrPriv = rrGetScrPriv(pScreen);

    provider = calloc(1, sizeof(RRProviderRec) + nameLength + 1);
    if (!provider)
        return NULL;

    provider->id = FakeClientID(0);
    provider->pScreen = pScreen;
    provider->name = (char *) (provider + 1);
    provider->nameLength = nameLength;
    memcpy(provider->name, name, nameLength);
    provider->name[nameLength] = '\0';
    provider->changed = FALSE;

    if (!AddResource (provider->id, RRProviderType, (pointer) provider))
        return NULL;
    pScrPriv->provider = provider;
    return provider;
}

/*
 * Destroy a provider at shutdown
 */
void
RRProviderDestroy (RRProviderPtr provider)
{
    FreeResource (provider->id, 0);
}

void
RRProviderSetCapabilities(RRProviderPtr provider, uint32_t capabilities)
{
    provider->capabilities = capabilities;
}

static int
RRProviderDestroyResource (pointer value, XID pid)
{
    RRProviderPtr provider = (RRProviderPtr)value;
    ScreenPtr pScreen = provider->pScreen;

    if (pScreen)
    {
        rrScrPriv(pScreen);

        if (pScrPriv->rrProviderDestroy)
            (*pScrPriv->rrProviderDestroy)(pScreen, provider);
        pScrPriv->provider = NULL;
    }
    free(provider);
    return 1;
}

Bool
RRProviderInit(void)
{
    RRProviderType = CreateNewResourceType(RRProviderDestroyResource, "Provider");
    if (!RRProviderType)
        return FALSE;

    return TRUE;
}

extern _X_EXPORT Bool
RRProviderLookup(XID id, RRProviderPtr *provider_p)
{
    int rc = dixLookupResourceByType((void **)provider_p, id,
                                   RRProviderType, NullClient, DixReadAccess);
    if (rc == Success)
        return TRUE;
    return FALSE;
}

void
RRDeliverProviderEvent(ClientPtr client, WindowPtr pWin, RRProviderPtr provider)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    xRRProviderChangeNotifyEvent pe;

    rrScrPriv(pScreen);


        pe.type = RRNotify + RREventBase;
        pe.subCode = RRNotify_ProviderChange;
        pe.timestamp = pScrPriv->lastSetTime.milliseconds;
        pe.window = pWin->drawable.id;
        pe.provider = provider->id;


    WriteEventsToClient(client, 1, (xEvent *) &pe);
}
